#include "stdIncl.h"

#pragma once
class BotControl
{
public:

	BotControl();

	void connectPort();

	bool move(int nTicks);
	bool turn(int nAngle);
	int getDistance();

	~BotControl(void);
	
private:

	struct ThParam
	{
		HANDLE hSerial;
		HANDLE hEvent1;
		
		OVERLAPPED overlappedW;
		OVERLAPPED overlappedR;

		string readMSG;
	};

	ThParam* Param;
	
	wstring getPortConnect(map<wstring,wstring> ports);	
	wstring findeCOMPort();
	
	string setStep(int nAngle, const double Konst);

	string getMovementCMD(string com);
	string getSensorCMD(string command);
	string getModeCMD (int mode);
	
	static DWORD WINAPI readCOM(LPVOID pParam);
	string sendCMD(string cmd);
};

