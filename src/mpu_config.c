/*
 * mpu_config.c
 *
 *  Created on: Mar 8, 2023
 *      Author: a5123412
 */
#include "mpu_config.h"

#define CTRL_PV 1

#define CTRL_HN 1

#define ARM_MPU_CTRL(PV, HN) (((PV) << MPU_CTRL_PRIVDEFENA_Pos) | ((HN) << MPU_CTRL_HFNMIENA_Pos))

#define MPU_ARMV8M_MAIR_ATTR_DEVICE_VAL             0x04	// Device memory -nGnRE (non-gathering, non-Reordering, Early Write Acknowledgement)

#define MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX             0

#define MPU_ARMV8M_MAIR_ATTR_CODE_VAL               0xAA	// Normal memory,(cacheability:) outer/inner write-through non-transient, Read allocate

#define MPU_ARMV8M_MAIR_ATTR_CODE_IDX               1

#define MPU_ARMV8M_MAIR_ATTR_DATA_VAL               0xFF	// Normal memory,(cacheability:) outer/inner write-back non-transient, Read/Write allocate

#define MPU_ARMV8M_MAIR_ATTR_DATA_IDX               2

#define MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_VAL        0x44	// Normal memory, Outer non-cacheable, Normal memory, Inner non-cacheable.

#define MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX        3

#if ENABLE_MPU_CONFIG_FOR_EDMAC
#define MEMORY_AREA_START_ADDRESS					(specified address)
#define MEMORY_AREA_END_ADDRESS						(specified address)

const ARM_MPU_Region_t mpuTable_for_edmac[1] = {
    //                       BASE          SH              RO   NP   XN
    { .RBAR = ARM_MPU_RBAR(MEMORY_AREA_START_ADDRESS, ARM_MPU_SH_NON, 0UL, 1UL, 1UL),
    //                       LIMIT         ATTR
      .RLAR = ARM_MPU_RLAR(MEMORY_AREA_END_ADDRESS, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX) },
};

void mpu_config_edmac(void)
{
	ARM_MPU_Disable();

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX, MPU_ARMV8M_MAIR_ATTR_DEVICE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_CODE_IDX, MPU_ARMV8M_MAIR_ATTR_CODE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATA_IDX, MPU_ARMV8M_MAIR_ATTR_DATA_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_VAL);

	ARM_MPU_Load(0, mpuTable_for_edmac, 1);

	ARM_MPU_Enable(ARM_MPU_CTRL(CTRL_PV, CTRL_HN));
}
#endif

#if ENABLE_MPU_CONFIG_FOR_QSPI

const ARM_MPU_Region_t mpuTable_for_qspi[2] = {
	//                         BASE          SH              RO   NP   XN
	{ .RBAR = ARM_MPU_RBAR(0x60000000UL, ARM_MPU_SH_NON, 0UL, 1UL, 1UL),
	//                        LIMIT         ATTR
	  .RLAR = ARM_MPU_RLAR(0x63FFFFFFUL, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX) },
	//                         BASE          SH              RO   NP   XN
	{ .RBAR = ARM_MPU_RBAR(0x64000000UL, ARM_MPU_SH_NON, 0UL, 1UL, 1UL),
	//                         LIMIT         ATTR
	  .RLAR = ARM_MPU_RLAR(0x67FFFFFFUL, MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX) }
};
void mpu_config_quadspi(void)
{
	ARM_MPU_Disable();

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX, MPU_ARMV8M_MAIR_ATTR_DEVICE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_CODE_IDX, MPU_ARMV8M_MAIR_ATTR_CODE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATA_IDX, MPU_ARMV8M_MAIR_ATTR_DATA_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_VAL);

	ARM_MPU_Load(0, mpuTable_for_qspi, 2);

	ARM_MPU_Enable(ARM_MPU_CTRL(CTRL_PV, CTRL_HN));
}
#endif

#if ENABLE_MPU_CONFIG_FOR_SRAM
#define SRAM_AREA_START_ADDRESS				(0x2000db98)
#define SRAM_AREA_END_ADDRESS				(0x2000e057)

const ARM_MPU_Region_t mpuTable_for_sram_area[1] = {
    //                       BASE          SH              RO   NP   XN
    { .RBAR = ARM_MPU_RBAR(SRAM_AREA_START_ADDRESS, ARM_MPU_SH_NON, 0UL, 1UL, 1UL),
    //                       LIMIT         ATTR
      .RLAR = ARM_MPU_RLAR(SRAM_AREA_END_ADDRESS, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX) },
};

void mpu_config_sram_area(void)
{
	ARM_MPU_Disable();

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DEVICE_IDX, MPU_ARMV8M_MAIR_ATTR_DEVICE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_CODE_IDX, MPU_ARMV8M_MAIR_ATTR_CODE_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATA_IDX, MPU_ARMV8M_MAIR_ATTR_DATA_VAL);

	ARM_MPU_SetMemAttr(MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_IDX, MPU_ARMV8M_MAIR_ATTR_DATANOCACHE_VAL);

	ARM_MPU_Load(0, mpuTable_for_sram_area, 1);

	ARM_MPU_Enable(ARM_MPU_CTRL(CTRL_PV, CTRL_HN));
}
#endif



