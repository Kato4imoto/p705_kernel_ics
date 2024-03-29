/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/pm_qos_params.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/camera.h>
#include <mach/clk.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>


/* MIPI	CSI controller registers */
#define	MIPI_PHY_CONTROL		0x00000000
#define	MIPI_PROTOCOL_CONTROL		0x00000004
#define	MIPI_INTERRUPT_STATUS		0x00000008
#define	MIPI_INTERRUPT_MASK		0x0000000C
#define	MIPI_CAMERA_CNTL		0x00000024
#define	MIPI_CALIBRATION_CONTROL	0x00000018
#define	MIPI_PHY_D0_CONTROL2		0x00000038
#define	MIPI_PHY_D1_CONTROL2		0x0000003C
#define	MIPI_PHY_D2_CONTROL2		0x00000040
#define	MIPI_PHY_D3_CONTROL2		0x00000044
#define	MIPI_PHY_CL_CONTROL		0x00000048
#define	MIPI_PHY_D0_CONTROL		0x00000034
#define	MIPI_PHY_D1_CONTROL		0x00000020
#define	MIPI_PHY_D2_CONTROL		0x0000002C
#define	MIPI_PHY_D3_CONTROL		0x00000030
#define	MIPI_PWR_CNTL			0x00000054

#define MIPI_IMASK_ERROR_OCCUR                0xF01FFFC0
#define MIPI_IMASK_CLK_ULPM_ENTRY             (0x00000001<<0)
#define MIPI_IMASK_CLK_ULPM_EXIT              (0x00000001<<1)
#define MIPI_IMASK_DATA_ULPM_ENTRY            (0x00000001<<2)
#define MIPI_IMASK_DATA_ULPM_EXIT             (0x00000001<<3)
#define MIPI_IMASK_CLK_START                  (0x00000001<<4)
#define MIPI_IMASK_CLK_STOP                   (0x00000001<<5)
#define MIPI_IMASK_ERR_SOT                    (0x00000001<<6)
#define MIPI_IMASK_ERR_SOT_SYNC               (0x00000001<<7)
#define MIPI_IMASK_CLK_CTL_ERROR              (0x00000001<<8)
#define MIPI_IMASK_DATA_CTL_ERROR             (0x00000001<<9)
#define MIPI_IMASK_CLK_CMM_ERROR              (0x00000001<<10)
#define MIPI_IMASK_DATA_CMM_ERROR             (0x00000001<<11)
#define MIPI_IMASK_DL0_SYNC_ERROR             (0x00000001<<12)
#define MIPI_IMASK_DL1_SYNC_ERROR             (0x00000001<<13)
#define MIPI_IMASK_DL2_SYNC_ERROR             (0x00000001<<14)
#define MIPI_IMASK_DL3_SYNC_ERROR             (0x00000001<<15)
#define MIPI_IMASK_ECC_ERROR                  (0x00000001<<16)
#define MIPI_IMASK_CRC_ERROR                  (0x00000001<<17)
#define MIPI_IMASK_FRAME_SYNC_ERROR           (0x00000001<<18)
#define MIPI_IMASK_ID_ERROR                   (0x00000001<<19)
#define MIPI_IMASK_EOT_ERROR                  (0x00000001<<20)
#define MIPI_IMASK_SW_RST_DONE                (0x00000001<<21)
#define MIPI_IMASK_SHORT_PACKET_CAPTURE_DONE  (0x00000001<<22)
#define MIPI_IMASK_CAL_DONE                   (0x00000001<<23)
#define MIPI_IMASK_DL0_FIFO_OVERFLOW          (0x00000001<<28)
#define MIPI_IMASK_DL1_FIFO_OVERFLOW          (0x00000001<<29)
#define MIPI_IMASK_DL2_FIFO_OVERFLOW          (0x00000001<<30)
#define MIPI_IMASK_DL3_FIFO_OVERFLOW          (0x00000001<<31)

/*
 * MIPI_PROTOCOL_CONTROL register bits to enable/disable the features of
 * CSI Rx Block
 */

/* DPCM scheme */
#define	MIPI_PROTOCOL_CONTROL_DPCM_SCHEME_SHFT			0x1e
/* SW_RST to issue a SW reset to the CSI core */
#define	MIPI_PROTOCOL_CONTROL_SW_RST_BMSK			0x8000000
/* To Capture Long packet Header Info in MIPI_PROTOCOL_STATUS register */
#define	MIPI_PROTOCOL_CONTROL_LONG_PACKET_HEADER_CAPTURE_BMSK	0x200000
/* Data format for unpacking purpose */
#define	MIPI_PROTOCOL_CONTROL_DATA_FORMAT_SHFT			0x13
/* Enable decoding of payload based on data type filed of packet hdr */
#define	MIPI_PROTOCOL_CONTROL_DECODE_ID_BMSK			0x00000
/* Enable error correction on packet headers */
#define	MIPI_PROTOCOL_CONTROL_ECC_EN_BMSK			0x20000

/*
 * MIPI_CALIBRATION_CONTROL register contains control info for
 * calibration impledence controller
*/

/* Enable bit for calibration pad */
#define	MIPI_CALIBRATION_CONTROL_SWCAL_CAL_EN_SHFT		0x16
/* With SWCAL_STRENGTH_OVERRIDE_EN, SW_CAL_EN and MANUAL_OVERRIDE_EN
 * the hardware calibration circuitry associated with CAL_SW_HW_MODE
 * is bypassed
*/
#define	MIPI_CALIBRATION_CONTROL_SWCAL_STRENGTH_OVERRIDE_EN_SHFT	0x15
/* To indicate the Calibration process is in the control of HW/SW */
#define	MIPI_CALIBRATION_CONTROL_CAL_SW_HW_MODE_SHFT		0x14
/* When this is set the strength value of the data and clk lane impedence
 * termination is updated with MANUAL_STRENGTH settings and calibration
 * sensing logic is idle.
*/
#define	MIPI_CALIBRATION_CONTROL_MANUAL_OVERRIDE_EN_SHFT	0x7

/* Data lane0 control */
/* T-hs Settle count value  for Rx */
#define	MIPI_PHY_D0_CONTROL2_SETTLE_COUNT_SHFT			0x18
/* Rx termination control */
#define	MIPI_PHY_D0_CONTROL2_HS_TERM_IMP_SHFT			0x10
/* LP Rx enable */
#define	MIPI_PHY_D0_CONTROL2_LP_REC_EN_SHFT			0x4
/*
 * Enable for error in sync sequence
 * 1 - one bit error in sync seq
 * 0 - requires all 8 bit correct seq
*/
#define	MIPI_PHY_D0_CONTROL2_ERR_SOT_HS_EN_SHFT			0x3

/* Comments are same as D0 */
#define	MIPI_PHY_D1_CONTROL2_SETTLE_COUNT_SHFT			0x18
#define	MIPI_PHY_D1_CONTROL2_HS_TERM_IMP_SHFT			0x10
#define	MIPI_PHY_D1_CONTROL2_LP_REC_EN_SHFT			0x4
#define	MIPI_PHY_D1_CONTROL2_ERR_SOT_HS_EN_SHFT			0x3

/* Comments are same as D0 */
#define	MIPI_PHY_D2_CONTROL2_SETTLE_COUNT_SHFT			0x18
#define	MIPI_PHY_D2_CONTROL2_HS_TERM_IMP_SHFT			0x10
#define	MIPI_PHY_D2_CONTROL2_LP_REC_EN_SHFT			0x4
#define	MIPI_PHY_D2_CONTROL2_ERR_SOT_HS_EN_SHFT			0x3

/* Comments are same as D0 */
#define	MIPI_PHY_D3_CONTROL2_SETTLE_COUNT_SHFT			0x18
#define	MIPI_PHY_D3_CONTROL2_HS_TERM_IMP_SHFT			0x10
#define	MIPI_PHY_D3_CONTROL2_LP_REC_EN_SHFT			0x4
#define	MIPI_PHY_D3_CONTROL2_ERR_SOT_HS_EN_SHFT			0x3

/* PHY_CL_CTRL programs the parameters of clk lane of CSIRXPHY */
/* HS Rx termination control */
#define	MIPI_PHY_CL_CONTROL_HS_TERM_IMP_SHFT			0x18
/* Start signal for T-hs delay */
#define	MIPI_PHY_CL_CONTROL_LP_REC_EN_SHFT			0x2

/* PHY DATA lane 0 control */
/*
 * HS RX equalizer strength control
 * 00 - 0db 01 - 3db 10 - 5db 11 - 7db
*/
#define	MIPI_PHY_D0_CONTROL_HS_REC_EQ_SHFT			0x1c

/* PHY DATA lane 1 control */
/* Shutdown signal for MIPI clk phy line */
#define	MIPI_PHY_D1_CONTROL_MIPI_CLK_PHY_SHUTDOWNB_SHFT		0x9
/* Shutdown signal for MIPI data phy line */
#define	MIPI_PHY_D1_CONTROL_MIPI_DATA_PHY_SHUTDOWNB_SHFT	0x8

#define MSM_AXI_QOS_PREVIEW 200000
#define MSM_AXI_QOS_SNAPSHOT 200000
#define MSM_AXI_QOS_RECORDING 200000

#define MIPI_PWR_CNTL_ENA	0x07
#define MIPI_PWR_CNTL_DIS	0x0

static struct clk *camio_cam_clk;
static struct clk *camio_vfe_clk;
static struct clk *camio_csi_src_clk;
static struct clk *camio_csi0_vfe_clk;
static struct clk *camio_csi1_vfe_clk;
static struct clk *camio_csi0_clk;
static struct clk *camio_csi1_clk;
static struct clk *camio_csi0_pclk;
static struct clk *camio_csi1_pclk;

static struct msm_camera_io_ext camio_ext;
static struct msm_camera_io_clk camio_clk;
static struct platform_device *camio_dev;
void __iomem *csibase;
void __iomem *appbase;



void msm_io_w(u32 data, void __iomem *addr)
{
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	writel((data), (addr));
}

u32 msm_io_r(void __iomem *addr)
{
	uint32_t data = readl(addr);
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	return data;
}

int msm_camio_vfe_clk_rate_set(int rate)
{
	int rc = 0;
	struct clk *clk = camio_vfe_clk;
	if (rate > clk_get_rate(clk))
		rc = clk_set_rate(clk, rate);
	return rc;
}

int msm_camio_clk_enable(enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;

	switch (clktype) {
	case CAMIO_CAM_MCLK_CLK:
		clk = clk_get(NULL, "cam_m_clk");
		camio_cam_clk = clk;
		msm_camio_clk_rate_set_2(clk, camio_clk.mclk_clk_rate);
		break;
	case CAMIO_VFE_CLK:
		clk = clk_get(NULL, "vfe_clk");
		camio_vfe_clk = clk;
		msm_camio_clk_rate_set_2(clk, camio_clk.vfe_clk_rate);
		break;
	case CAMIO_CSI0_VFE_CLK:
		clk = clk_get(&camio_dev->dev, "csi_vfe_clk");
		camio_csi0_vfe_clk = clk;
		break;
	case CAMIO_CSI1_VFE_CLK:
		clk = clk_get(NULL, "csi_vfe_clk");
		camio_csi1_vfe_clk = clk;
		break;
	case CAMIO_CSI_SRC_CLK:
		clk = clk_get(NULL, "csi_src_clk");
		camio_csi_src_clk = clk;
		break;
	case CAMIO_CSI0_CLK:
		clk = clk_get(&camio_dev->dev, "csi_clk");
		camio_csi0_clk = clk;
		msm_camio_clk_rate_set_2(clk, 400000000);
		break;
	case CAMIO_CSI1_CLK:
		clk = clk_get(NULL, "csi_clk");
		camio_csi1_clk = clk;
		break;
	case CAMIO_CSI0_PCLK:
		clk = clk_get(&camio_dev->dev, "csi_pclk");
		camio_csi0_pclk = clk;
		break;
	case CAMIO_CSI1_PCLK:
		clk = clk_get(NULL, "csi_pclk");
		camio_csi1_pclk = clk;
		break;
	default:
		break;
	}

	if (!IS_ERR(clk))
		clk_enable(clk);
	else
		rc = -1;
	return rc;
}

int msm_camio_clk_disable(enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;

	switch (clktype) {
	case CAMIO_CAM_MCLK_CLK:
		clk = camio_cam_clk;
		break;
	case CAMIO_VFE_CLK:
		clk = camio_vfe_clk;
		break;
	case CAMIO_CSI_SRC_CLK:
		clk = camio_csi_src_clk;
		break;
	case CAMIO_CSI0_VFE_CLK:
		clk = camio_csi0_vfe_clk;
		break;
	case CAMIO_CSI1_VFE_CLK:
		clk = camio_csi1_vfe_clk;
		break;
	case CAMIO_CSI0_CLK:
		clk = camio_csi0_clk;
		break;
	case CAMIO_CSI1_CLK:
		clk = camio_csi1_clk;
		break;
	case CAMIO_CSI0_PCLK:
		clk = camio_csi0_pclk;
		break;
	case CAMIO_CSI1_PCLK:
		clk = camio_csi1_pclk;
		break;
	default:
		break;
	}

	if (!IS_ERR(clk)) {
		clk_disable(clk);
		clk_put(clk);
	} else
		rc = -1;
	return rc;
}

void msm_camio_clk_rate_set(int rate)
{
	struct clk *clk = camio_cam_clk;
	clk_set_rate(clk, rate);
}

void msm_camio_clk_rate_set_2(struct clk *clk, int rate)
{
	clk_set_rate(clk, rate);
}

static irqreturn_t msm_io_csi_irq(int irq_num, void *data)
{
	uint32_t irq;

	irq = msm_io_r(csibase + MIPI_INTERRUPT_STATUS);
//	printk("%s MIPI_INTERRUPT_STATUS = 0x%x\n", __func__, irq);
#if 0
	if (irq & MIPI_IMASK_ERROR_OCCUR) {
			CDBG("%s MIPI_INTERRUPT_STATUS = 0x%x\n", __func__, irq);
			if (irq & MIPI_IMASK_ERR_SOT)
					CDBG("[CAM]msm_io_csi_irq: SOT error\n");
			if (irq & MIPI_IMASK_ERR_SOT_SYNC)
					CDBG("[CAM]msm_io_csi_irq: SOT SYNC error\n");
			if (irq & MIPI_IMASK_CLK_CTL_ERROR)
					CDBG("[CAM]msm_io_csi_irq: Clock lane ULPM mode sequence or command error\n");
			if (irq & MIPI_IMASK_DATA_CTL_ERROR)
					CDBG("[CAM]msm_io_csi_irq: Data lane ULPM mode sequence or command error\n");
			if (irq & MIPI_IMASK_CLK_CMM_ERROR) /* defeatured */
					CDBG("[CAM]msm_io_csi_irq: Common mode error detected by PHY CLK lane\n");
			if (irq & MIPI_IMASK_DATA_CMM_ERROR) /* defeatured */
					CDBG("[CAM]msm_io_csi_irq: Common mode error detected by PHY data lane\n");
			if (irq & MIPI_IMASK_DL0_SYNC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: An error occured while synchronizing data " \
							"from PHY to VFE clock domain on data lane 0\n");
			if (irq & MIPI_IMASK_DL1_SYNC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: An error occured while synchronizing data " \
							"from PHY to VFE clock domain on data lane 1\n");
			if (irq & MIPI_IMASK_DL2_SYNC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: An error occured while synchronizing data " \
							"from PHY to VFE clock domain on data lane 2\n");
			if (irq & MIPI_IMASK_DL3_SYNC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: An error occured while synchronizing data " \
							"from PHY to VFE clock domain on data lane 3\n");
			if (irq & MIPI_IMASK_ECC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: ECC error\n");
			if (irq & MIPI_IMASK_CRC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: CRC error\n");
			if (irq & MIPI_IMASK_FRAME_SYNC_ERROR)
					CDBG("[CAM]msm_io_csi_irq: FS not paired with FE\n");
			if (irq & MIPI_IMASK_ID_ERROR)
					CDBG("[CAM]msm_io_csi_irq: Long packet ID not defined\n");
			if (irq & MIPI_IMASK_EOT_ERROR)
					CDBG("[CAM]msm_io_csi_irq: The received data is less than the value indicated by WC\n");
			if (irq & MIPI_IMASK_DL0_FIFO_OVERFLOW)
					CDBG("[CAM]msm_io_csi_irq: Data lane 0 FIFO overflow\n");
			if (irq & MIPI_IMASK_DL1_FIFO_OVERFLOW)
					CDBG("[CAM]msm_io_csi_irq: Data lane 1 FIFO overflow\n");
			if (irq & MIPI_IMASK_DL2_FIFO_OVERFLOW)
					CDBG("[CAM]msm_io_csi_irq: Data lane 2 FIFO overflow\n");
			if (irq & MIPI_IMASK_DL3_FIFO_OVERFLOW)
					CDBG("[CAM]msm_io_csi_irq: Data lane 3 FIFO overflow\n");
	}
#endif
	msm_io_w(irq, csibase + MIPI_INTERRUPT_STATUS);

	/* TODO: Needs to send this info to upper layers */
	if ((irq >> 19) & 0x1)
		pr_info("Unsupported packet format is received\n");
	return IRQ_HANDLED;
}

int msm_camio_enable(struct platform_device *pdev)
{
	int rc = 0;
	const struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camio_dev = pdev;
	camio_ext = camdev->ioext;
	camio_clk = camdev->ioclk;

	msm_camio_clk_enable(CAMIO_VFE_CLK);
	msm_camio_clk_enable(CAMIO_CSI0_VFE_CLK);
	msm_camio_clk_enable(CAMIO_CSI1_VFE_CLK);
	msm_camio_clk_enable(CAMIO_CSI0_CLK);
	msm_camio_clk_enable(CAMIO_CSI1_CLK);
	msm_camio_clk_enable(CAMIO_CSI0_PCLK);
	msm_camio_clk_enable(CAMIO_CSI1_PCLK);

	csibase = ioremap(camio_ext.csiphy, camio_ext.csisz);
	if (!csibase) {
		rc = -ENOMEM;
		pr_err("###### csi busy ######\n");
		goto csi_busy;
	}
	rc = request_irq(camio_ext.csiirq, msm_io_csi_irq,
				IRQF_TRIGGER_HIGH, "csi", 0);

	if (rc < 0) {
		pr_err("###### request irq failed ######\n");
		goto csi_irq_fail;
	}

	appbase = ioremap(camio_ext.appphy,
		camio_ext.appsz);
	if (!appbase) {
		rc = -ENOMEM;
		goto csi_irq_fail;
	}
	return 0;

csi_irq_fail:
	iounmap(csibase);
csi_busy:
	msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	msm_camio_clk_disable(CAMIO_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI1_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_CLK);
	msm_camio_clk_disable(CAMIO_CSI1_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_PCLK);
	msm_camio_clk_disable(CAMIO_CSI1_PCLK);
	camdev->camera_gpio_off();
	return rc;
}

static void msm_camio_csi_disable(void)
{
	uint32_t val;

	val = 0x0;
	CDBG("%s MIPI_PHY_D0_CONTROL2 val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PHY_D0_CONTROL2);
	msm_io_w(val, csibase + MIPI_PHY_D1_CONTROL2);
 	msm_io_w(val, csibase + MIPI_PHY_D2_CONTROL2);
 	msm_io_w(val, csibase + MIPI_PHY_D3_CONTROL2);

	CDBG("%s MIPI_PHY_CL_CONTROL val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PHY_CL_CONTROL);
	msleep(20);

	free_irq(camio_ext.csiirq, 0);
	iounmap(csibase);
	iounmap(appbase);
	
}

void msm_camio_disable(struct platform_device *pdev)
{
	CDBG("disable mipi\n");
	msm_camio_csi_disable();

	CDBG("disable clocks\n");

	msm_camio_clk_disable(CAMIO_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_CLK);
	msm_camio_clk_disable(CAMIO_CSI1_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI1_VFE_CLK);
	msm_camio_clk_disable(CAMIO_CSI0_PCLK);
	msm_camio_clk_disable(CAMIO_CSI1_PCLK);
}

int msm_camio_sensor_clk_on(struct platform_device *pdev)
{
	int rc = 0;
	const struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camio_dev = pdev;
	camio_ext = camdev->ioext;
	camio_clk = camdev->ioclk;

	rc = camdev->camera_gpio_on();
	if (rc < 0)
		return rc;
#ifdef CONFIG_HI542
	msleep(5);  /* LGE_CHANGE donghyun.kwon@lge.com (20120203) : change power sequence*/
#endif
	return msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
}

int msm_camio_sensor_clk_off(struct platform_device *pdev)
{
	const struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

#ifdef CONFIG_HI542
	int rc=0;
	rc = msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
	msleep(20);
	camdev->camera_gpio_off();
	return rc;
#else
	camdev->camera_gpio_off();
	return msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
#endif

}

void msm_camio_vfe_blk_reset(void)
{
	uint32_t val;

	/* do apps reset */
	val = readl_relaxed(appbase + 0x00000210);
	val |= 0x1;
	writel_relaxed(val, appbase + 0x00000210);
	usleep_range(10000, 11000);

	val = readl_relaxed(appbase + 0x00000210);
	val &= ~0x1;
	writel_relaxed(val, appbase + 0x00000210);
	usleep_range(10000, 11000);

	/* do axi reset */
	val = readl_relaxed(appbase + 0x00000208);
	val |= 0x1;
	writel_relaxed(val, appbase + 0x00000208);
	usleep_range(10000, 11000);

	val = readl_relaxed(appbase + 0x00000208);
	val &= ~0x1;
	writel_relaxed(val, appbase + 0x00000208);
	mb();
	usleep_range(10000, 11000);
	return;
}

int msm_camio_probe_on(struct platform_device *pdev)
{
	int rc = 0;
	const struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camio_dev = pdev;
	camio_ext = camdev->ioext;
	camio_clk = camdev->ioclk;

	msm_camio_clk_enable(CAMIO_CSI0_PCLK);
	msm_camio_clk_enable(CAMIO_CSI1_PCLK);

	rc = camdev->camera_gpio_on();
	if (rc < 0)
		return rc;
#ifdef CONFIG_HI542
	msleep(5);  /* LGE_CHANGE donghyun.kwon@lge.com (20120203) : change power sequence*/
#endif
	return msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
}

int msm_camio_probe_off(struct platform_device *pdev)
{
	const struct msm_camera_sensor_info *sinfo = pdev->dev.platform_data;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;
	camdev->camera_gpio_off();

	csibase = ioremap(camdev->ioext.csiphy, camdev->ioext.csisz);
	if (!csibase) {
		pr_err("ioremap failed for CSIBASE\n");
		goto ioremap_fail;
	}
	msm_io_w(MIPI_PWR_CNTL_DIS, csibase + MIPI_PWR_CNTL);
	iounmap(csibase);
ioremap_fail:
	msm_camio_clk_disable(CAMIO_CSI0_PCLK);
	msm_camio_clk_disable(CAMIO_CSI1_PCLK);
	return msm_camio_clk_disable(CAMIO_CAM_MCLK_CLK);
}

int msm_camio_csi_config(struct msm_camera_csi_params *csi_params)
{
	int rc = 0;
	uint32_t val = 0;

	CDBG("msm_camio_csi_config\n");

	/* Enable error correction for DATA lane. Applies to all data lanes */
	msm_io_w(0x4, csibase + MIPI_PHY_CONTROL);

	msm_io_w(MIPI_PROTOCOL_CONTROL_SW_RST_BMSK,
		csibase + MIPI_PROTOCOL_CONTROL);

	val = MIPI_PROTOCOL_CONTROL_LONG_PACKET_HEADER_CAPTURE_BMSK |
		MIPI_PROTOCOL_CONTROL_DECODE_ID_BMSK |
		MIPI_PROTOCOL_CONTROL_ECC_EN_BMSK;
	val |= (uint32_t)(csi_params->data_format) <<
		MIPI_PROTOCOL_CONTROL_DATA_FORMAT_SHFT;
	val |= csi_params->dpcm_scheme <<
		MIPI_PROTOCOL_CONTROL_DPCM_SCHEME_SHFT;
	CDBG("%s MIPI_PROTOCOL_CONTROL val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PROTOCOL_CONTROL);

	val = (csi_params->settle_cnt <<
		MIPI_PHY_D0_CONTROL2_SETTLE_COUNT_SHFT) |
		(0x0F << MIPI_PHY_D0_CONTROL2_HS_TERM_IMP_SHFT) |
		(0x1 << MIPI_PHY_D0_CONTROL2_LP_REC_EN_SHFT) |
		(0x1 << MIPI_PHY_D0_CONTROL2_ERR_SOT_HS_EN_SHFT);
	CDBG("%s MIPI_PHY_D0_CONTROL2 val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PHY_D0_CONTROL2);
	msm_io_w(val, csibase + MIPI_PHY_D1_CONTROL2);
	msm_io_w(0x0, csibase + MIPI_PHY_D2_CONTROL2);
	msm_io_w(0x0, csibase + MIPI_PHY_D3_CONTROL2);

	val = (0x0F << MIPI_PHY_CL_CONTROL_HS_TERM_IMP_SHFT) |
		(0x1 << MIPI_PHY_CL_CONTROL_LP_REC_EN_SHFT);
	CDBG("%s MIPI_PHY_CL_CONTROL val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PHY_CL_CONTROL);

	val = 0 << MIPI_PHY_D0_CONTROL_HS_REC_EQ_SHFT;
	msm_io_w(val, csibase + MIPI_PHY_D0_CONTROL);

	val = (0x1 << MIPI_PHY_D1_CONTROL_MIPI_CLK_PHY_SHUTDOWNB_SHFT) |
		(0x1 << MIPI_PHY_D1_CONTROL_MIPI_DATA_PHY_SHUTDOWNB_SHFT);
	CDBG("%s MIPI_PHY_D1_CONTROL val=0x%x\n", __func__, val);
	msm_io_w(val, csibase + MIPI_PHY_D1_CONTROL);

	msm_io_w(0x00000000, csibase + MIPI_PHY_D2_CONTROL);
	msm_io_w(0x00000000, csibase + MIPI_PHY_D3_CONTROL);

	/* program number of lanes and lane mapping */
	switch (csi_params->lane_cnt) {
	case 1:
		msm_io_w(csi_params->lane_assign << 8 | 0x4,
			csibase + MIPI_CAMERA_CNTL);
		break;
	case 2:
		msm_io_w(csi_params->lane_assign << 8 | 0x5,
			csibase + MIPI_CAMERA_CNTL);
		break;
	case 3:
		msm_io_w(csi_params->lane_assign << 8 | 0x6,
			csibase + MIPI_CAMERA_CNTL);
		break;
	case 4:
		msm_io_w(csi_params->lane_assign << 8 | 0x7,
			csibase + MIPI_CAMERA_CNTL);
		break;
	}

	msm_io_w(0xFFFFF3FF, csibase + MIPI_INTERRUPT_MASK);
	/*clear IRQ bits - write 1 clears the status*/
	msm_io_w(0xFFFFF3FF, csibase + MIPI_INTERRUPT_STATUS);

	return rc;
}

void msm_camio_set_perf_lvl(enum msm_bus_perf_setting perf_setting)
{
	switch (perf_setting) {
	case S_INIT:
		add_axi_qos();
		break;
	case S_PREVIEW:
		update_axi_qos(MSM_AXI_QOS_PREVIEW);
		break;
	case S_VIDEO:
		update_axi_qos(MSM_AXI_QOS_RECORDING);
		break;
	case S_CAPTURE:
		update_axi_qos(MSM_AXI_QOS_SNAPSHOT);
		break;
	case S_DEFAULT:
		update_axi_qos(PM_QOS_DEFAULT_VALUE);
		break;
	case S_EXIT:
		release_axi_qos();
		break;
	default:
		CDBG("%s: INVALID CASE\n", __func__);
	}
}
