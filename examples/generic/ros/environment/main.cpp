/*
 * Copyright (c) 2017, Sascha Schade
 * Copyright (c) 2018, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * Example to demonstrate periodically publishing environment data
 * (temperature, pressure, humidity) to ROS as stamped messages.
 *
 * Connect a Bosch BME280 sensor and a SSD1306 (128x64) display to I2C.
 */

#include <modm/board.hpp>
#include <modm/processing.hpp>

#include <sensor_msgs/Temperature.h>
#include <sensor_msgs/FluidPressure.h>
#include <sensor_msgs/RelativeHumidity.h>

#include "thread_bme280.hpp"
#include "thread_display.hpp"

#include "hardware.hpp"

#include <ros/node_handle.h>
#include <modm/communication/ros.hpp>

#undef	MODM_LOG_LEVEL
#define	MODM_LOG_LEVEL modm::log::DISABLED

// Define which UART is used for communication to and from ROS serial
// When using the STlink Uart on Nucleo boards logging must be disabled.
using RosSerialUart = BufferedUart<UsartHal2, UartTxBuffer<512>, UartRxBuffer<512>>;

namespace ros
{
	using modmHardware = ModmHardware<RosSerialUart>;
	using ModmNodeHandle = NodeHandle_<modmHardware>;
}

ros::ModmNodeHandle nh;
modm::Fiber fiber_ros([]
{
	while (true)
	{
		nh.spinOnce();
		modm::this_fiber::yield();
	}
});

sensor_msgs::Temperature      temperature_msg;
sensor_msgs::FluidPressure    pressure_msg;
sensor_msgs::RelativeHumidity humidity_msg;

ros::Publisher pub_temperature("/environment/temperature",       &temperature_msg);
ros::Publisher pub_pressure   ("/environment/pressure",          &pressure_msg);
ros::Publisher pub_humidity   ("/environment/relative_humidity", &humidity_msg);

Bme280Thread bme280thread;
DisplayThread display_thread;

modm::Fiber fiber_display([]
{
	modm::ShortPeriodicTimer timer(1s);
	while(true)
	{
		timer.wait();
		bme280thread.startMeasurement();
		modm::this_fiber::poll([&]{ return bme280thread.isNewDataAvailable(); });

		Board::LedGreen::toggle();
		int32_t temp = bme280thread.getTemperature();
		int32_t pres = bme280thread.getPressure();
		int32_t humi = bme280thread.getHumidity();

		// Convert to standard ROS units.
		temperature_msg.temperature = temp / 100.0;
		temperature_msg.variance = 1.0;
		temperature_msg.header.stamp = nh.now();

		pressure_msg.fluid_pressure = pres / 1000.0;
		pressure_msg.variance = 100;
		pressure_msg.header.stamp = nh.now();

		humidity_msg.relative_humidity = humi / 100000.0;
		humidity_msg.variance = 0.03;
		humidity_msg.header.stamp = nh.now();

		pub_temperature.publish(&temperature_msg);
		pub_pressure.publish(&pressure_msg);
		pub_humidity.publish(&humidity_msg);

		display_thread.setSeq(temperature_msg.header.stamp.sec);
		display_thread.setTemp(temp);
		display_thread.setPres(pres);
		display_thread.setHumi(humi);

		// Do not enable when STlink UART is used for rosserial
		MODM_LOG_DEBUG.printf("Temp = %6.2f\n", temp / double(100.0));
	}
});

// ----------------------------------------------------------------------------
int
main()
{
	Board::initialize();

	// Reinit onboard UART to 1 Mbps
	// Do not use it for logging because this will destroy ROS serial messages.
	Board::stlink::Uart::initialize<Board::SystemClock, 1_MBd>();

	MyI2cMaster::connect<Board::D14::Sda, Board::D15::Scl>();
	MyI2cMaster::initialize<Board::SystemClock, 100_kHz>();

	Board::LedGreen::set();

	nh.initNode();

	nh.advertise(pub_temperature);
	nh.advertise(pub_pressure);
	nh.advertise(pub_humidity);

	modm::fiber::Scheduler::run();

	return 0;
}
