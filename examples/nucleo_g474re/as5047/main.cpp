// coding: utf-8
/*
 * Copyright (c) 2024, Henrik Hose
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <modm/board.hpp>
#include <modm/driver/encoder/as5047.hpp>

using SpiMaster = SpiMaster1;

using Cs = modm::platform::GpioB10;
using Mosi = modm::platform::GpioB5;
using Miso = modm::platform::GpioB4;
using Sck = modm::platform::GpioB3;

modm::as5047::Data data{0};
modm::As5047<SpiMaster, Cs> encoder{data};

int
main()
{
	Board::initialize();

	Cs::setOutput(modm::Gpio::High);

	SpiMaster::connect<Miso::Miso, Mosi::Mosi, Sck::Sck>();
	SpiMaster::initialize<Board::SystemClock, 1.3_MHz>();

	MODM_LOG_INFO << "==========AS5047 Test==========" << modm::endl;
	MODM_LOG_DEBUG << "Debug logging here" << modm::endl;
	MODM_LOG_INFO << "Info logging here" << modm::endl;
	MODM_LOG_WARNING << "Warning logging here" << modm::endl;
	MODM_LOG_ERROR << "Error logging here" << modm::endl;
	MODM_LOG_INFO << "===============================" << modm::endl;

	while (true)
	{
		encoder.read();

		MODM_LOG_INFO << "\nNew readout:" << modm::endl;
		MODM_LOG_INFO << "  angle degree: " << data.toDegree() << " degrees" << modm::endl;
		MODM_LOG_INFO << "     angle rad: " << data.toRadian() << " radians" << modm::endl;
		MODM_LOG_INFO << "     angle raw: " << data.data << modm::endl;

		modm::delay(500ms);
	}

	return 0;
}
