/**
 * @file icm20948.c
 * @brief ICM20948 Sensor Driver
 *
 * This module provides functions to communicate with the ICM20948 sensor.
 * The ICM20948 is a combination of a 3-axis gyroscope, 3-axis accelerometer,
 * and a 3-axis magnetometer.
 *
 * The functions are categorized into:
 * 1. Static utility functions for lower level SPI communication.
 * 2. Initialization functions for setting up the ICM20948 sensor and the AK09916 magnetometer.
 * 3. Data reading functions that fetch raw data from the sensor and convert them
 *    to real-world units like degrees per second (dps), Gs, and micro Tesla (uT).
 *
 * Features:
 * - Reading from specific registers of the gyroscope, accelerometer/magnetometer
 * - Writing to specific registers of the gyroscope, accelerometer/magnetometer
 * - Initializing the gyroscope, accelerometer, and magnetometer.
 * - Reading raw data from the gyroscope, accelerometer, and magnetometer.
 * - Converting raw data into dps for gyroscope, Gs for accelerometer, and uT for magnetometer.
 *
 * @note For full sensor specification, consult the ICM20948 datasheet.
 * @note Make sure to configure the SPI communication parameters before using this driver.
 *
 * @author Xu Mujie
 * @date 2023.09.05
 * @version 1.0
 */


#include "icm20948.h"


static float gyro_scale_factor;
static float accel_scale_factor;


/* Static Functions */
static void     cs_high();
static void     cs_low();

static void     select_user_bank(userbank ub);

//read and write data to icm20948 register, especially for accelerometer and gyroscope
static uint8_t  read_single_icm20948_reg(userbank ub, uint8_t reg);
static void     write_single_icm20948_reg(userbank ub, uint8_t reg, uint8_t val);
static uint8_t* read_multiple_icm20948_reg(userbank ub, uint8_t reg, uint8_t len);
static void     write_multiple_icm20948_reg(userbank ub, uint8_t reg, uint8_t* val, uint8_t len);

//read and write data to ak09918, the magnetometer
static uint8_t  read_single_ak09916_reg(uint8_t reg);
static void     write_single_ak09916_reg(uint8_t reg, uint8_t val);
static uint8_t* read_multiple_ak09916_reg(uint8_t reg, uint8_t len);


/* Main Functions */
/**
 * @brief Initialize the ICM20948 sensor.
 *
 * This function initializes the ICM20948 sensor by configuring various settings
 *
 * @return None.
 */
void icm20948_init()
{
    //who_am_i check
	while(!icm20948_who_am_i());
    // IMU reset, page 37
	icm20948_device_reset();
	// Exit from sleep mode, selecting the clock 37
	icm20948_wakeup();
	icm20948_clock_source(1);

	//output data rate start time alignment, page 63
	icm20948_odr_align_enable();
    //Reset I2C Slave module and put the serial interface in SPI mode only.
	icm20948_spi_slave_enable();

    // Enable digital low pass filter for gyroscope and accelerometer
	icm20948_gyro_low_pass_filter(0);
	icm20948_accel_low_pass_filter(0);

    // Set sample rate(ODR) for gyroscope and accelerometer. scaler =10, means both sample at 102.3Hz
	icm20948_gyro_sample_rate_divider(10);
	icm20948_accel_sample_rate_divider(10);

    //ICM gyroscope and accelerometer bias cancellation function
	icm20948_gyro_calibration();
	icm20948_accel_calibration();

    //Choose full-scale range for gyroscope and accelerometer
	icm20948_gyro_full_scale_select(_2000dps);
	icm20948_accel_full_scale_select(_16g);
}

/**
 * @brief Initialize the ak09916 magnetometer.
 *
 * This function initializes the ak09916 magnetometer by configuring various settings
 *
 * @return None.
 */
void ak09916_init()
{
    //reset i2c master
	icm20948_i2c_master_reset();
	//Enable the I2C Master I/F module
	icm20948_i2c_master_enable();
	//Enable the I2C Master clock, input 7 means setting the I2C Master clock frequency at 400kHz.
	icm20948_i2c_master_clk_frq(7);

    //magnetometer who am i check
	while(!ak09916_who_am_i());


	// LP_CONFIG: ODR is determined by I2C_MST_ODR_CONFIG register, page 37
	// I2C_MST_ODR_CONFIG: 1.1 kHz/(2^3) = 136 Hz, page 68
	ak09916_lp_config();
	//ak09916 reset
	ak09916_soft_reset();
	//Choose the magnetometer to Continuous Measurement Mode 4 at 100Hz.
	//This makes sure the sensor is measured periodically in 100Hz.
	ak09916_operation_mode_setting(continuous_measurement_100hz);
}

/**
 * @brief Read gyroscope data
 * @return None.
 */
void icm20948_gyro_read(axises* data)
{
	uint8_t* temp = read_multiple_icm20948_reg(ub_0, B0_GYRO_XOUT_H, 6);

	data->x = (int16_t)(temp[0] << 8 | temp[1]);
	data->y = (int16_t)(temp[2] << 8 | temp[3]);
	data->z = (int16_t)(temp[4] << 8 | temp[5]);
}
/**
 * @brief Read accelerometer data
 * @return None.
 */
void icm20948_accel_read(axises* data)
{
	uint8_t* temp = read_multiple_icm20948_reg(ub_0, B0_ACCEL_XOUT_H, 6);

	data->x = (int16_t)(temp[0] << 8 | temp[1]);
	data->y = (int16_t)(temp[2] << 8 | temp[3]);
	data->z = (int16_t)(temp[4] << 8 | temp[5]) + accel_scale_factor;
	// Add scale factor because calibraiton function offset gravity acceleration.
}

/**
 * @brief Read magnetometer data.
 *
 * Data ready and overflow tests implemented first. If the magnetometer passes the two tests first,
    read the data from registers. If system pass these two tests, a true will be returned at the end of the function.
 *
 * @return true/false depending on whether have passed the tests.
 */
bool ak09916_mag_read(axises* data)
{
	uint8_t* temp;
	uint8_t drdy, hofl;	// data ready, overflow

	drdy = read_single_ak09916_reg(MAG_ST1) & 0x01;
	if(!drdy){
		printf("data is not ready\n");
		return false;
	}

	temp = read_multiple_ak09916_reg(MAG_HXL, 6);

	hofl = read_single_ak09916_reg(MAG_ST2) & 0x08;
	if(hofl){
		printf("data is overflow\n");
		return false;
	}

	data->x = (int16_t)(temp[1] << 8 | temp[0]);
	data->y = (int16_t)(temp[3] << 8 | temp[2]);
	data->z = (int16_t)(temp[5] << 8 | temp[4]);

	return true;
}
/**
 * @brief Read gyroscope data in dps
 * @return None.
 */
void icm20948_gyro_read_dps(axises* data)
{
	icm20948_gyro_read(data);

	data->x /= gyro_scale_factor;
	data->y /= gyro_scale_factor;
	data->z /= gyro_scale_factor;
}

/**
 * @brief Read accelerometer data in g
 * @return None.
 */
void icm20948_accel_read_g(axises* data)
{
	icm20948_accel_read(data);

	data->x /= accel_scale_factor;
	data->y /= accel_scale_factor;
	data->z /= accel_scale_factor;
}
/**
 * @brief Read magnetometer data in uT
 * Read magnetometer should make sure no overflow and data is ready for reading
 * @return None.
 */
bool ak09916_mag_read_uT(axises* data)
//void ak09916_mag_read_uT(axises* data)
{
	axises temp;
	bool new_data = ak09916_mag_read(&temp);
//	ak09916_mag_read(&temp);
	if(!new_data)	return false;

	data->x = (float)(temp.x * 0.15);
	data->y = (float)(temp.y * 0.15);
	data->z = (float)(temp.z * 0.15);

	printf("magnetometer : %f, %f, and %f\n", data ->x,
			data ->y, data ->z);

	return true;
}
/**
 * @brief Read all data, accelerometer, gyroscope and magnetometer data altogether
 * Make sure all data is in the standard units, and no overflow and data is ready for data collection
 * And store on the data in the struct, icm_20948_data type.
 * @return result(icm_20948_data).
 */
icm_20948_data read_all_data(void)
//uint8_t read_all_data(icm_20948_data* data)
{
	icm_20948_data result;
	axises my_gyro;
	axises my_accel;
	axises my_mag;

	icm20948_gyro_read(&my_gyro);

	my_gyro.x /= gyro_scale_factor;
	my_gyro.y /= gyro_scale_factor;
	my_gyro.z /= gyro_scale_factor;

	icm20948_accel_read(&my_accel);

	my_accel.x /= accel_scale_factor;
	my_accel.y /= accel_scale_factor;
	my_accel.z /= accel_scale_factor;

	axises temp;
	bool new_data = ak09916_mag_read(&temp);
//	ak09916_mag_read(&temp);
	printf("new data is %d\n", new_data);
	if(!new_data)
	{
		printf("data not ready/ overflow for magnetometer\n");
		while(1);
	}
	else
	{
		printf("magnetometer reading finished.\n");
	}

	my_mag.x = (float)(temp.x * 0.15);
	my_mag.y = (float)(temp.y * 0.15);
	my_mag.z = (float)(temp.z * 0.15);


    result.x_magnet = my_mag.x;
    result.y_magnet = my_mag.y;
    result.z_magnet = my_mag.z;

    result.x_accel = my_accel.x;
    result.y_accel = my_accel.y;
    result.z_accel = my_accel.z;

    result.x_gyro = my_gyro.x;
    result.y_gyro = my_gyro.y;
    result.z_gyro = my_gyro.z;

    printf("accelerometer : %f, %f, and %f \n", result.x_accel,
           result.y_accel, result.z_accel);
    printf("gyroscope : %f, %f, and %f \n", result.x_gyro,
           result.y_gyro, result.z_gyro);
    printf("magnetometer : %f, %f, and %f\n", result.x_magnet,
           result.y_magnet, result.z_magnet);

    return result;

}
/**
 * @brief who_am_i check for icm20948
 * @return true/false.
 */
bool icm20948_who_am_i()
{
	uint8_t icm20948_id = read_single_icm20948_reg(ub_0, B0_WHO_AM_I);
	printf("the icm20948 who am i is: 0x%x\n",icm20948_id);
	if(icm20948_id == ICM20948_ID){
		printf("Data matches, icm20948 identity verified.\n");
		return true;
	}
	else{
		printf("Data not matched, icm20948 incorrect.\n");
		return false;
	}
}
/**
 * @brief who_am_i check for ak09916
 * @return true/false.
 */
bool ak09916_who_am_i()
{
	uint8_t ak09916_id = read_single_ak09916_reg(MAG_WIA2);
	printf("the ak09916_id who am i is: 0x%x\n", ak09916_id);
	if(ak09916_id == AK09916_ID)
	{
		printf("Data matches, ak09916 identity verified.\n");
		return true;
	}
	else
	{
		printf("Data not matched, ak09916 incorrect.\n");
		return false;
	}
}

void icm20948_device_reset()
{
	write_single_icm20948_reg(ub_0, B0_PWR_MGMT_1, 0x80 | 0x41);
	HAL_Delay(100);
}
/**
 * @brief Configure low pass filter for magnetometer and i2c master odr rate
 * @return None.
 */
void ak09916_lp_config()
{
	// LP_CONFIG: ODR is determined by I2C_MST_ODR_CONFIG register, page 37
	write_single_icm20948_reg(ub_0, B0_LP_CONFIG, 0x40);
	HAL_Delay(100);
	// I2C_MST_ODR_CONFIG: 1.1 kHz/(2^3) = 136 Hz, page 68
	write_single_icm20948_reg(ub_3, B3_I2C_MST_ODR_CONFIG, 0x03);
	HAL_Delay(100);
}
/**
 * @brief reset ak09916
 * @return None.
 */
void ak09916_soft_reset()
{
	write_single_ak09916_reg(MAG_CNTL3, 0x01);
	HAL_Delay(100);
}
/**
 * @brief icm20948 exit from sleep mode
 * @return None.
 */
void icm20948_wakeup()
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_PWR_MGMT_1);
	new_val &= 0xBF;

	write_single_icm20948_reg(ub_0, B0_PWR_MGMT_1, new_val);
	HAL_Delay(100);
}
/**
 * @brief icm20948 enter sleep mode
 * @return None.
 */
void icm20948_sleep()
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_PWR_MGMT_1);
	new_val |= 0x40;

	write_single_icm20948_reg(ub_0, B0_PWR_MGMT_1, new_val);
	HAL_Delay(100);
}

/**
 * @brief who_am_i check for icm20948
 * @return None.
 */
void icm20948_spi_slave_enable()
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_USER_CTRL);
	new_val |= 0x10;

	write_single_icm20948_reg(ub_0, B0_USER_CTRL, new_val);
}

/**
 * @brief icm20948 i2c master reset
 * @return None.
 */
void icm20948_i2c_master_reset()
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_USER_CTRL);
	new_val |= 0x02;

	write_single_icm20948_reg(ub_0, B0_USER_CTRL, new_val);
}
/**
 * @brief icm20948 i2c master enable
 * @return None.
 */
void icm20948_i2c_master_enable()
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_USER_CTRL);
	new_val |= 0x20;

	write_single_icm20948_reg(ub_0, B0_USER_CTRL, new_val);
	HAL_Delay(100);
}
/**
 * @brief icm20948 i2c master clock frequency: chosen to be 7KHz
 * @return None.
 */
void icm20948_i2c_master_clk_frq(uint8_t config)
{
	uint8_t new_val = read_single_icm20948_reg(ub_3, B3_I2C_MST_CTRL);
	new_val |= config;

	write_single_icm20948_reg(ub_3, B3_I2C_MST_CTRL, new_val);
}

/**
 * @brief icm20948 clock source chose
 * @return None.
 */
void icm20948_clock_source(uint8_t source)
{
	uint8_t new_val = read_single_icm20948_reg(ub_0, B0_PWR_MGMT_1);
	new_val |= source;

	write_single_icm20948_reg(ub_0, B0_PWR_MGMT_1, new_val);
}
/**
 * @brief icm20948 output data rate start time alignment, page 63
 * @return None.
 */
void icm20948_odr_align_enable()
{
	write_single_icm20948_reg(ub_2, B2_ODR_ALIGN_EN, 0x01);
}
/**
 * @brief enable gyroscope digital low pass filter
 * @return None.
 */
void icm20948_gyro_low_pass_filter(uint8_t config)
{
	uint8_t new_val = read_single_icm20948_reg(ub_2, B2_GYRO_CONFIG_1);
	new_val |= config << 3;

	write_single_icm20948_reg(ub_2, B2_GYRO_CONFIG_1, new_val);
}
/**
 * @brief enable accelerometer digital low pass filter
 * @return None.
 */
void icm20948_accel_low_pass_filter(uint8_t config)
{
	uint8_t new_val = read_single_icm20948_reg(ub_2, B2_ACCEL_CONFIG);
	new_val |= config << 3;

	write_single_icm20948_reg(ub_2, B2_GYRO_CONFIG_1, new_val);
}
/**
 * @brief icm20948 sample rate divider, different scaler gives different odr rate.
 * We set it to be 100Hz, chose scaler to be 10
 * @return None.
 */
void icm20948_gyro_sample_rate_divider(uint8_t divider)
{
	write_single_icm20948_reg(ub_2, B2_GYRO_SMPLRT_DIV, divider);
}
/**
 * @brief icm20948 sample rate divider, different scaler gives different odr rate.
 * We set it to be 100Hz, chose scaler to be 10
 * @return None.
 */
void icm20948_accel_sample_rate_divider(uint16_t divider)
{
	uint8_t divider_1 = (uint8_t)(divider >> 8);
	uint8_t divider_2 = (uint8_t)(0x0F & divider);

	write_single_icm20948_reg(ub_2, B2_ACCEL_SMPLRT_DIV_1, divider_1);
	write_single_icm20948_reg(ub_2, B2_ACCEL_SMPLRT_DIV_2, divider_2);
}
/**
 * @brief icm20948 continuous measurement mode
 * Sensor is measured periodically in 100Hz
 * @return None.
 */
void ak09916_operation_mode_setting(operation_mode mode)
{
	write_single_ak09916_reg(MAG_CNTL2, mode);
	HAL_Delay(100);
}

/**
 * @brief remove gyroscope calibration
 * Take 100 measurements, take average, divide by four to get the gyroscope bias and update to the bias registers
 * @return None.
 */
void icm20948_gyro_calibration()
{
	axises temp;
	int32_t gyro_bias[3] = {0};
	uint8_t gyro_offset[6] = {0};

    //take 100 gyroscope measurements
	for(int i = 0; i < 100; i++)
	{
		icm20948_gyro_read(&temp);
		gyro_bias[0] += temp.x;
		gyro_bias[1] += temp.y;
		gyro_bias[2] += temp.z;
	}

    //Divide 100 to get the gyroscope average reading
	gyro_bias[0] /= 100;
	gyro_bias[1] /= 100;
	gyro_bias[2] /= 100;

	// Construct the gyro biases for push to the hardware gyro bias registers,
	// which are reset to zero upon device startup.
	// Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format.
	// Biases are additive, so change sign on calculated average gyro biases
	gyro_offset[0] = (-gyro_bias[0] / 4  >> 8) & 0xFF;
	gyro_offset[1] = (-gyro_bias[0] / 4)       & 0xFF;
	gyro_offset[2] = (-gyro_bias[1] / 4  >> 8) & 0xFF;
	gyro_offset[3] = (-gyro_bias[1] / 4)       & 0xFF;
	gyro_offset[4] = (-gyro_bias[2] / 4  >> 8) & 0xFF;
	gyro_offset[5] = (-gyro_bias[2] / 4)       & 0xFF;

	write_multiple_icm20948_reg(ub_2, B2_XG_OFFS_USRH, gyro_offset, 6);
}

/**
 * @brief remove accelerometer calibration
 * Take 100 accel measurements, take average, preserve the LSB to ensure it is unchanged, devide the accelerometers by
 * eight and update to the bias registers
 * @return None.
 */
void icm20948_accel_calibration()
{
	axises temp;
	uint8_t* temp2;
	uint8_t* temp3;
	uint8_t* temp4;

	int32_t accel_bias[3] = {0};
	int32_t accel_bias_reg[3] = {0};
	uint8_t accel_offset[6] = {0};

    //Take 100 accel measurements
	for(int i = 0; i < 100; i++)
	{
		icm20948_accel_read(&temp);
		accel_bias[0] += temp.x;
		accel_bias[1] += temp.y;
		accel_bias[2] += temp.z;
	}

    //Divide by 100 get the average accelerometer readings
	accel_bias[0] /= 100;
	accel_bias[1] /= 100;
	accel_bias[2] /= 100;

    //Define mask bit
	uint8_t mask_bit[3] = {0, 0, 0};

    // From these registers, it extracts the LSB and stores it in ’mask bit’ to ensure its value remains unchanged
    // Divide the average bias by 8, and update into these hardware accelerometer bias registers.
	temp2 = read_multiple_icm20948_reg(ub_1, B1_XA_OFFS_H, 2);
	accel_bias_reg[0] = (int32_t)(temp2[0] << 8 | temp2[1]);
	mask_bit[0] = temp2[1] & 0x01;

	temp3 = read_multiple_icm20948_reg(ub_1, B1_YA_OFFS_H, 2);
	accel_bias_reg[1] = (int32_t)(temp3[0] << 8 | temp3[1]);
	mask_bit[1] = temp3[1] & 0x01;

	temp4 = read_multiple_icm20948_reg(ub_1, B1_ZA_OFFS_H, 2);
	accel_bias_reg[2] = (int32_t)(temp4[0] << 8 | temp4[1]);
	mask_bit[2] = temp4[1] & 0x01;

	accel_bias_reg[0] -= (accel_bias[0] / 8);
	accel_bias_reg[1] -= (accel_bias[1] / 8);
	accel_bias_reg[2] -= (accel_bias[2] / 8);

	accel_offset[0] = (accel_bias_reg[0] >> 8) & 0xFF;
  	accel_offset[1] = (accel_bias_reg[0])      & 0xFE;
	accel_offset[1] = accel_offset[1] | mask_bit[0];

	accel_offset[2] = (accel_bias_reg[1] >> 8) & 0xFF;
  	accel_offset[3] = (accel_bias_reg[1])      & 0xFE;
	accel_offset[3] = accel_offset[3] | mask_bit[1];

	accel_offset[4] = (accel_bias_reg[2] >> 8) & 0xFF;
	accel_offset[5] = (accel_bias_reg[2])      & 0xFE;
	accel_offset[5] = accel_offset[5] | mask_bit[2];

	write_multiple_icm20948_reg(ub_1, B1_XA_OFFS_H, &accel_offset[0], 2);
	write_multiple_icm20948_reg(ub_1, B1_YA_OFFS_H, &accel_offset[2], 2);
	write_multiple_icm20948_reg(ub_1, B1_ZA_OFFS_H, &accel_offset[4], 2);
}

/**
 * @brief Gyroscope full-scale range choices
 * @return None.
 */
void icm20948_gyro_full_scale_select(gyro_full_scale full_scale)
{
	uint8_t new_val = read_single_icm20948_reg(ub_2, B2_GYRO_CONFIG_1);

	switch(full_scale)
	{
		case _250dps :
			new_val |= 0x00;
			gyro_scale_factor = 131.0;
			break;
		case _500dps :
			new_val |= 0x02;
			gyro_scale_factor = 65.5;
			break;
		case _1000dps :
			new_val |= 0x04;
			gyro_scale_factor = 32.8;
			break;
		case _2000dps :
			new_val |= 0x06;
			gyro_scale_factor = 16.4;
			break;
	}

	write_single_icm20948_reg(ub_2, B2_GYRO_CONFIG_1, new_val);
}

/**
 * @brief Accelerometer full-scale range choices
 * @return None.
 */
void icm20948_accel_full_scale_select(accel_full_scale full_scale)
{
	uint8_t new_val = read_single_icm20948_reg(ub_2, B2_ACCEL_CONFIG);

	switch(full_scale)
	{
		case _2g :
			new_val |= 0x00;
			accel_scale_factor = 16384;
			break;
		case _4g :
			new_val |= 0x02;
			accel_scale_factor = 8192;
			break;
		case _8g :
			new_val |= 0x04;
			accel_scale_factor = 4096;
			break;
		case _16g :
			new_val |= 0x06;
			accel_scale_factor = 2048;
			break;
	}

	write_single_icm20948_reg(ub_2, B2_ACCEL_CONFIG, new_val);
}


/* Static Functions */
//toggle the gpio output pin(CS pin) to high
static void cs_high()
{
	HAL_GPIO_WritePin(ICM20948_SPI_CS_PIN_PORT, ICM20948_SPI_CS_PIN_NUMBER, SET);
}
//toggle the gpio output pin(CS pin) to low
static void cs_low()
{
	HAL_GPIO_WritePin(ICM20948_SPI_CS_PIN_PORT, ICM20948_SPI_CS_PIN_NUMBER, RESET);
}
//Select userbank
static void select_user_bank(userbank ub)
{
	uint8_t write_reg[2];
	write_reg[0] = WRITE | REG_BANK_SEL;
	write_reg[1] = ub;

	cs_low();
	HAL_SPI_Transmit(ICM20948_SPI, write_reg, 2, 10);
	cs_high();
}

//SPI read ICM20948 registers, transmit register address and receive data
static uint8_t read_single_icm20948_reg(userbank ub, uint8_t reg)
{
	uint8_t read_reg = READ | reg;
	uint8_t reg_val;
	select_user_bank(ub);

	cs_low();
	HAL_SPI_Transmit(ICM20948_SPI, &read_reg, 1, 1000);
	HAL_SPI_Receive(ICM20948_SPI, &reg_val, 1, 1000);
	cs_high();

	return reg_val;
}

//SPI write ICM20948 registers, transmit register address and data together
static void write_single_icm20948_reg(userbank ub, uint8_t reg, uint8_t val)
{
	uint8_t write_reg[2];
	write_reg[0] = WRITE | reg;
	write_reg[1] = val;

	select_user_bank(ub);

	cs_low();
	HAL_SPI_Transmit(ICM20948_SPI, write_reg, 2, 1000);
	cs_high();
}

//SPI read multiple registers
static uint8_t* read_multiple_icm20948_reg(userbank ub, uint8_t reg, uint8_t len)
{
	uint8_t read_reg = READ | reg;
	static uint8_t reg_val[6];
	select_user_bank(ub);

	cs_low();
	HAL_SPI_Transmit(ICM20948_SPI, &read_reg, 1, 1000);
	HAL_SPI_Receive(ICM20948_SPI, reg_val, len, 1000);
	cs_high();

	return reg_val;
}
//SPI write multiple registers
static void write_multiple_icm20948_reg(userbank ub, uint8_t reg, uint8_t* val, uint8_t len)
{
	uint8_t write_reg = WRITE | reg;
	select_user_bank(ub);

	cs_low();
	HAL_SPI_Transmit(ICM20948_SPI, &write_reg, 1, 1000);
	HAL_SPI_Transmit(ICM20948_SPI, val, len, 1000);
	cs_high();
}
/**
 * @brief Read ak09916 single byte
 * enable I2C Master, ak09916 = I2C slave, ICM = I2C Master. I2C Master comm. all uses SPI comm. to configurate
 * @return ak09916 measurements. HAL_Delay() added to make sure no data overflow.
 */
static uint8_t read_single_ak09916_reg(uint8_t reg)
{
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_ADDR, READ | MAG_SLAVE_ADDR);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_REG, reg);
	HAL_Delay(50);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_CTRL, 0x81);
	HAL_Delay(50);
	return read_single_icm20948_reg(ub_0, B0_EXT_SLV_SENS_DATA_00);
}
/**
 * @brief write ak09916 single byte
 * enable I2C Master, ak09916 = I2C slave, ICM = I2C Master. I2C Master comm. all uses SPI comm. to configurate
 * B3_I2C_SLV0_CTRL: Enable reading data from this ak09916 slave at the sample rate and storing data at the first available
 * HAL_Delay() added to make sure no data overflow.
        EXT_SENS_DATA register, which is always EXT_SENS_DATA_00 for I2C slave 0.
        one bytes to be read from I2C slave 0.
 * @return one byte reading from EXT_SENS_DATA register: the magnetometer measurement
 */
static void write_single_ak09916_reg(uint8_t reg, uint8_t val)
{
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_ADDR, WRITE | MAG_SLAVE_ADDR);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_REG, reg);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_DO, val);
	//	Enable and single data write
	HAL_Delay(50);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_CTRL, 0x81);
	HAL_Delay(50);
}
/**
 * @brief Read ak09916 multiple byte
 * HAL_Delay() added to make sure no data overflow.
 * @return ak09916 measurements in multiple bytes, no. of bytes determined by the input value.
 */
static uint8_t* read_multiple_ak09916_reg(uint8_t reg, uint8_t len)
{
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_ADDR, READ | MAG_SLAVE_ADDR);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_REG, reg);
	HAL_Delay(50);
	write_single_icm20948_reg(ub_3, B3_I2C_SLV0_CTRL, 0x80 | len);
	HAL_Delay(50);
	return read_multiple_icm20948_reg(ub_0, B0_EXT_SLV_SENS_DATA_00, len);
}

