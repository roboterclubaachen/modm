/*
 * Copyright (c) 2016, Sascha Schade
 * Copyright (c) 2017, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/board.hpp>
#include <modm/debug/logger.hpp>

#include <modm/processing.hpp>

#include "hardware.hpp"

// ----------------------------------------------------------------------------
#include "threads.hpp"

Bmp180Thread bmp180Thread;
Bme280Thread bme280Thread;

// ----------------------------------------------------------------------------
// Set the log level
#undef	MODM_LOG_LEVEL
#define	MODM_LOG_LEVEL modm::log::DEBUG

// Create an IODeviceWrapper around the Uart Peripheral we want to use
using Usart2 = BufferedUart<UsartHal2, UartTxBuffer<256>>;
modm::IODeviceWrapper< Usart2, modm::IOBuffer::BlockIfFull > loggerDevice;
modm::IOStream stream(loggerDevice);

// Set all four logger streams to use the UART
modm::log::Logger modm::log::debug(loggerDevice);
modm::log::Logger modm::log::info(loggerDevice);
modm::log::Logger modm::log::warning(loggerDevice);
modm::log::Logger modm::log::error(loggerDevice);

// ----------------------------------------------------------------------------

using namespace Board;

int
main()
{
	Board::initialize();

	// ------------------------------------------------------------------------
	// I2C for sensors
	SensorsAI2cMaster::connect<GpioB6::Scl, GpioB7::Sda>();
	SensorsAI2cMaster::initialize<Board::SystemClock, 10_kHz>();

	SensorsBI2cMaster::connect<GpioB10::Scl, GpioB11::Sda>();
	SensorsBI2cMaster::initialize<Board::SystemClock, 10_kHz>();

	// ------------------------------------------------------------------------
	// initialize Uart2 for MODM_LOG_*
	Usart2::connect<GpioOutputA2::Tx>();
	Usart2::initialize<Board::SystemClock, 115200_Bd>();

	MODM_LOG_DEBUG << "Welcome to Environment Sensor Test" << modm::endl;

	modm::fiber::Scheduler::run();

	return 0;
}
