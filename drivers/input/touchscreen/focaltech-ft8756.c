// SPDX-License-Identifier: GPL-2.0
/*
 * FocalTech FT8756 touchscreen (TDDI, SPI, firmware downloaded by the host).
 *
 * The chip has no flash: after every reset the host loads the touch firmware
 * into its RAM through the ROM bootloader, then starts it. Protocol and
 * download sequence follow FocalTech's vendor driver ("Focaltech V3.x",
 * focaltech_spi.c / focaltech_flash.c, GPL-2.0), reduced to what one panel
 * needs: no gestures, no ESD check, no sysfs upgrade nodes.
 *
 * Measured on a TCL 30 SE (bit-banged too, so it is the chip and not the SPI
 * controller): the answer to a read command is clocked out in the NEXT frame,
 * seven bytes after its start. A lone read frame therefore returns the answer
 * to whatever was asked before it. The vendor driver sends single frames and
 * gets away with it because it re-reads the same register in loops; here a
 * read is two identical frames and the second one carries the data. The ROM's
 * checksum commands (0xcc/0xce/0xcd) never returned a checksum on this chip in
 * any frame order, so the download is verified by the firmware answering with
 * its chip ID instead.
 */

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/unaligned.h>

/* SPI frame: cmd, control, len_hi, len_lo, 3 dummy bytes, data [, crc16 on reads] */
#define FT_CTRL_WRITE		0x00
#define FT_CTRL_READ		0xa0	/* read + CRC */
#define FT_HDR			4
#define FT_DUMMY		3
#define FT_STATUS_ERR		0xa0	/* bits in rx[3] that mean "not accepted" */
#define FT_RETRIES		3
#define FT_CS_HIGH_US		150

#define FT_PACKET		4092	/* download chunk; the vendor's slow-path size */
#define FT_BUF			(FT_PACKET + 32)

/* ROM bootloader */
#define FT_CMD_BOOT		0x55
#define FT_CMD_READ_ID		0x90
#define FT_CMD_SET_ADDR		0xad
#define FT_CMD_WRITE		0xae
#define FT_CMD_START		0x08
#define FT_ROM_ID_H		0x87
#define FT_ROM_ID_L		0x56
#define FT_DRAM_BASE		0xd00000
#define FT_FW_INFO		0x100	/* be16 code words, ~code, 4 bytes, be16 data words, ~data */
#define FT_FW_MIN		0x120
#define FT_LEN_UNIT		2	/* lengths in the firmware header count 16-bit words */

/* Running firmware */
#define FT_REG_TOUCH		0x01
#define FT_REG_CHIP_ID		0xa3
#define FT_REG_FW_VER		0xa6
#define FT_MAX_POINTS		10
#define FT_POINT_LEN		6
/*
 * The chip queues every report as one fixed 90-byte packet (62 bytes of touch data plus gesture data), which
 * is what the vendor module reads (pnt_buf_size 91 - 1). Reading fewer bytes leaves the rest in the chip's
 * output queue and shifts every later answer; so does reading when no interrupt has announced a report.
 */
#define FT_TOUCH_LEN		90

struct ft8756 {
	struct spi_device *spi;
	struct input_dev *input;
	struct gpio_desc *reset;
	struct touchscreen_properties prop;
	struct mutex lock;		/* serialises SPI frames and the shared buffers */
	const char *fw_name;
	int dbg;
	u8 *tx, *rx;
};

static u16 ft8756_crc(const u8 *data, u32 len)
{
	u16 crc = 0xffff;
	u32 i;
	int j;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++)
			crc = (crc & 1) ? (crc >> 1) ^ 0x8408 : crc >> 1;
	}
	return crc;
}

/* Caller holds ts->lock. One full-duplex frame, retried while the chip reports busy/error. */
static int ft8756_frame(struct ft8756 *ts, u32 len, bool check_crc, u32 crc_from)
{
	struct spi_transfer xfer = { .tx_buf = ts->tx, .rx_buf = ts->rx, .len = len };
	int ret = -EIO, i;

	for (i = 0; i < FT_RETRIES; i++) {
		ret = spi_sync_transfer(ts->spi, &xfer, 1);
		if (!ret && !(ts->rx[3] & FT_STATUS_ERR)) {
			if (!check_crc)
				break;
			if (ft8756_crc(ts->rx + crc_from, len - crc_from - 2) ==
			    get_unaligned_le16(ts->rx + len - 2))
				break;
		}
		ret = -EIO;
		usleep_range(FT_CS_HIGH_US, FT_CS_HIGH_US + 50);
	}
	usleep_range(FT_CS_HIGH_US, FT_CS_HIGH_US + 50);
	return ret;
}

/* buf[0] is the command, the rest its payload */
static int ft8756_write(struct ft8756 *ts, const u8 *buf, u32 len)
{
	u32 datalen = len - 1, txlen = FT_HDR;
	int ret;

	if (!len || datalen > FT_PACKET + 8)
		return -EINVAL;

	mutex_lock(&ts->lock);
	memset(ts->tx, 0, FT_HDR + FT_DUMMY);
	ts->tx[0] = buf[0];
	ts->tx[1] = FT_CTRL_WRITE;
	put_unaligned_be16(datalen, ts->tx + 2);
	if (datalen) {
		memcpy(ts->tx + FT_HDR + FT_DUMMY, buf + 1, datalen);
		txlen += FT_DUMMY + datalen;
	}
	ret = ft8756_frame(ts, txlen, false, 0);
	mutex_unlock(&ts->lock);
	return ret;
}

static int ft8756_write_cmd(struct ft8756 *ts, u8 cmd)
{
	return ft8756_write(ts, &cmd, 1);
}

static int ft8756_read(struct ft8756 *ts, u8 cmd, u8 *data, u32 len)
{
	u32 dp = FT_HDR + FT_DUMMY, txlen = dp + len + 2;
	int ret;

	if (!len || txlen > FT_BUF)
		return -EINVAL;

	mutex_lock(&ts->lock);
	memset(ts->tx, 0, txlen);
	ts->tx[0] = cmd;
	ts->tx[1] = FT_CTRL_READ;
	put_unaligned_be16(len, ts->tx + 2);
	ret = ft8756_frame(ts, txlen, true, dp);
	/* Copy even on failure: the caller looks for the 0xef "no firmware" pattern. */
	memcpy(data, ts->rx + dp, len);
	mutex_unlock(&ts->lock);
	return ret;
}

/* Single read frame: only for the ROM's boot id, which is queued by the 0x55 command itself. */
static int ft8756_read_once(struct ft8756 *ts, u8 cmd, u8 *data, u32 len)
{
	u32 dp = FT_HDR + FT_DUMMY, txlen = dp + len + 2;
	int ret;

	mutex_lock(&ts->lock);
	memset(ts->tx, 0, txlen);
	ts->tx[0] = cmd;
	ts->tx[1] = FT_CTRL_READ;
	put_unaligned_be16(len, ts->tx + 2);
	ret = ft8756_frame(ts, txlen, true, dp);
	memcpy(data, ts->rx + dp, len);
	mutex_unlock(&ts->lock);
	return ret;
}

static void ft8756_reset(struct ft8756 *ts)
{
	gpiod_set_value_cansleep(ts->reset, 1);
	usleep_range(1000, 1500);
	gpiod_set_value_cansleep(ts->reset, 0);
}

static int ft8756_enter_boot(struct ft8756 *ts)
{
	u8 id[2] = { 0 };
	int i, j;

	for (i = 0; i < 30; i++) {
		ft8756_reset(ts);
		msleep(8 + i);
		for (j = 0; j < 3; j++) {
			if (ft8756_write_cmd(ts, FT_CMD_BOOT))
				continue;
			msleep(8);
			if (!ft8756_read_once(ts, FT_CMD_READ_ID, id, 2) &&
			    id[0] == FT_ROM_ID_H && id[1] == FT_ROM_ID_L)
				return 0;
		}
	}
	dev_err(&ts->spi->dev, "ROM bootloader did not answer (last id %02x %02x)\n", id[0], id[1]);
	return -ENODEV;
}

static int ft8756_ram_write(struct ft8756 *ts, u32 base, const u8 *buf, u32 len)
{
	u8 *cmd;
	u32 off, n;
	int ret = 0;

	cmd = kmalloc(FT_PACKET + 1, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	for (off = 0; off < len && !ret; off += n) {
		u8 addr[4] = { FT_CMD_SET_ADDR };

		n = min_t(u32, FT_PACKET, len - off);
		put_unaligned_be24(base + off, addr + 1);
		ret = ft8756_write(ts, addr, sizeof(addr));
		if (ret)
			break;
		cmd[0] = FT_CMD_WRITE;
		memcpy(cmd + 1, buf + off, n);
		ret = ft8756_write(ts, cmd, n + 1);
	}
	kfree(cmd);
	return ret;
}

static int ft8756_download_once(struct ft8756 *ts, const u8 *fw, size_t size)
{
	struct device *dev = &ts->spi->dev;
	u32 code, data;
	u16 len, nlen;
	int ret;

	len = get_unaligned_be16(fw + FT_FW_INFO);
	nlen = get_unaligned_be16(fw + FT_FW_INFO + 2);
	code = (u32)len * FT_LEN_UNIT;
	if ((u16)(len + nlen) != 0xffff || code < FT_FW_MIN || code > size) {
		dev_err(dev, "bad firmware header: code %04x/%04x, file %zu bytes\n", len, nlen, size);
		return -EINVAL;
	}
	len = get_unaligned_be16(fw + FT_FW_INFO + 8);
	nlen = get_unaligned_be16(fw + FT_FW_INFO + 10);
	data = ((u16)(len + nlen) == 0xffff) ? (u32)len * FT_LEN_UNIT : 0;
	if (code + data > size)
		return -EINVAL;

	ret = ft8756_enter_boot(ts);
	if (ret)
		return ret;

	ret = ft8756_ram_write(ts, 0, fw, code);
	if (!ret && data)
		ret = ft8756_ram_write(ts, FT_DRAM_BASE, fw + code, data);
	if (ret) {
		dev_err(dev, "RAM download failed: %d\n", ret);
		return ret;
	}
	ret = ft8756_write_cmd(ts, FT_CMD_START);
	if (ret)
		return ret;
	msleep(20);
	dev_dbg(dev, "firmware started: code %u bytes, data %u bytes\n", code, data);
	return 0;
}

static int ft8756_download(struct ft8756 *ts)
{
	struct device *dev = &ts->spi->dev;
	const struct firmware *fw;
	u8 id = 0, ver = 0;
	int ret, i;

	ret = request_firmware(&fw, ts->fw_name, dev);
	if (ret)
		return dev_err_probe(dev, ret, "firmware %s not available\n", ts->fw_name);

	ret = fw->size < FT_FW_MIN ? -EINVAL : -EIO;
	for (i = 0; i < 3 && ret == -EIO; i++)
		ret = ft8756_download_once(ts, fw->data, fw->size);
	release_firmware(fw);
	if (ret)
		return ret;

	for (i = 0; i < 20; i++) {
		if (!ft8756_read(ts, FT_REG_CHIP_ID, &id, 1) && id == FT_ROM_ID_H)
			break;
		msleep(10);
	}
	ft8756_read(ts, FT_REG_FW_VER, &ver, 1);
	dev_info(dev, "FT8756 firmware running: chip id %02x, fw version %02x\n", id, ver);
	return id == FT_ROM_ID_H ? 0 : -ENODEV;
}

static irqreturn_t ft8756_irq(int irq, void *dev_id)
{
	struct ft8756 *ts = dev_id;
	struct input_dev *input = ts->input;
	u8 buf[FT_TOUCH_LEN];
	int ret, i;

	/*
	 * One 90-byte read per interrupt, like the vendor driver, and never without an interrupt: the chip
	 * queues exactly one report packet per interrupt. A failed CRC is retried by ft8756_frame().
	 */
	ret = ft8756_read_once(ts, FT_REG_TOUCH, buf, sizeof(buf));
	if (buf[1] == 0xef && buf[2] == 0xef && buf[3] == 0xef) {
		/* The chip lost its firmware (panel reset, ESD): load it again. */
		dev_warn(&ts->spi->dev, "firmware lost, reloading\n");
		ft8756_download(ts);
		goto release_all;
	}
	if (ret)
		return IRQ_HANDLED;

	for (i = 0; i < FT_MAX_POINTS; i++) {
		const u8 *p = buf + 2 + i * FT_POINT_LEN;
		unsigned int id = p[2] >> 4, event = p[0] >> 6;

		if (id >= FT_MAX_POINTS)
			break;
		input_mt_slot(input, id);
		if (!input_mt_report_slot_state(input, MT_TOOL_FINGER, event != 1))
			continue;
		touchscreen_report_pos(input, &ts->prop, ((p[0] & 0x0f) << 8) | p[1],
				       ((p[2] & 0x0f) << 8) | p[3], true);
		input_report_abs(input, ABS_MT_TOUCH_MAJOR, p[5] ?: 9);
	}
release_all:
	input_mt_sync_frame(input);
	input_sync(input);
	return IRQ_HANDLED;
}

static int ft8756_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct input_dev *input;
	struct ft8756 *ts;
	int ret;

	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;
	ret = spi_setup(spi);
	if (ret)
		return ret;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;
	ts->spi = spi;
	mutex_init(&ts->lock);
	ts->tx = devm_kmalloc(dev, FT_BUF, GFP_KERNEL);
	ts->rx = devm_kmalloc(dev, FT_BUF, GFP_KERNEL);
	if (!ts->tx || !ts->rx)
		return -ENOMEM;

	ts->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ts->reset))
		return dev_err_probe(dev, PTR_ERR(ts->reset), "no reset gpio\n");

	ts->fw_name = "focaltech/ft8756.bin";
	device_property_read_string(dev, "firmware-name", &ts->fw_name);

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;
	ts->input = input;
	input->name = "FocalTech FT8756";
	input->id.bustype = BUS_SPI;
	input_set_abs_params(input, ABS_MT_POSITION_X, 0, 719, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y, 0, 1599, 0, 0);
	input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	touchscreen_parse_properties(input, true, &ts->prop);
	ret = input_mt_init_slots(input, FT_MAX_POINTS, INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
	if (ret)
		return ret;

	/*
	 * On the TCL 30 SE the chip arrives with its firmware already running (loaded before Linux starts)
	 * and the reset GPIO does not reset it. A download attempt then feeds 70 kB of firmware image to the
	 * live firmware as if it were commands and wrecks its SPI state, so only download when nothing answers.
	 */
	{
		u8 id = 0, ver = 0;

		if (!ft8756_read(ts, FT_REG_CHIP_ID, &id, 1) && id == FT_ROM_ID_H) {
			ft8756_read(ts, FT_REG_FW_VER, &ver, 1);
			dev_info(dev, "FT8756 firmware already running (version %02x), not downloading\n", ver);
		} else {
			ret = ft8756_download(ts);
			if (ret)
				return ret;
		}
	}

	ret = input_register_device(input);
	if (ret)
		return ret;

	return devm_request_threaded_irq(dev, spi->irq, NULL, ft8756_irq, IRQF_ONESHOT,
					 dev_name(dev), ts);
}

static const struct of_device_id ft8756_of_match[] = {
	{ .compatible = "focaltech,ft8756" },
	{ }
};
MODULE_DEVICE_TABLE(of, ft8756_of_match);

static const struct spi_device_id ft8756_spi_ids[] = {
	{ "ft8756" },
	{ }
};
MODULE_DEVICE_TABLE(spi, ft8756_spi_ids);

static struct spi_driver ft8756_driver = {
	.driver = {
		.name = "focaltech-ft8756",
		.of_match_table = ft8756_of_match,
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
	},
	.probe = ft8756_probe,
	.id_table = ft8756_spi_ids,
};
module_spi_driver(ft8756_driver);

MODULE_DESCRIPTION("FocalTech FT8756 SPI touchscreen (host-downloaded firmware)");
MODULE_LICENSE("GPL");
