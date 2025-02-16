#pragma once

struct DefConstructProbe {
	bool defaulted;

	DefConstructProbe()
			: defaulted(true) {}

	DefConstructProbe(int)
			: defaulted(false) {}
};
