#pragma once


namespace scomp {
	void getGlslangVersion(int &aMajor, int &aMinor, int &aPatch);

	void init();
	void finalize();
}