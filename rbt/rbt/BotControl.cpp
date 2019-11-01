#include "BotControl.h"

BotControl::BotControl()
{
	Param = new ThParam;
}

void BotControl::connectPort()
{

	wstring tmp = findeCOMPort();
	LPCTSTR sPortName = tmp.c_str();

	Param->hSerial = CreateFile(sPortName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);

	if(Param->hSerial==INVALID_HANDLE_VALUE)
	{
		if(GetLastError()==ERROR_FILE_NOT_FOUND)
		{
			cout << "serial port does not exist.\n";
		}
		cout << "some other error occurred.\n";
	}


	DCB dcbSerialParams = {0};
	dcbSerialParams.DCBlength=sizeof(dcbSerialParams);

	if (!GetCommState(Param->hSerial, &dcbSerialParams))
	{
		cout << "getting state error\n";
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.fBinary = TRUE;

	dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
	dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
	dcbSerialParams.fDsrSensitivity = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fAbortOnError = FALSE;

	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if(SetCommState(Param->hSerial, &dcbSerialParams) == 0)
	{
		cout << "error setting serial port state\n";
	}

	COMMTIMEOUTS comtimeouts; 
        comtimeouts.ReadIntervalTimeout = 0;
        comtimeouts.ReadTotalTimeoutMultiplier = 0;
        comtimeouts.ReadTotalTimeoutConstant =  0;
        comtimeouts.WriteTotalTimeoutMultiplier = 0;
        comtimeouts.WriteTotalTimeoutConstant = 0;

	SetCommTimeouts(Param->hSerial, &comtimeouts);

	SetupComm(Param->hSerial,2000,2000);

	PurgeComm(Param->hSerial,PURGE_RXCLEAR);
}

bool BotControl::move(int nTicks)
{
	bool rtn = 0;

	ostringstream st;
	st << nTicks;

	string s = st.str();
	string command = "forward 1 5 " + s + " ";

	string cmd = getMovementCMD(command);

	string read = "";
	read = sendCMD(cmd);

	if (read != "err")
	{
		rtn = 1;
	}
	
	return rtn;
}

bool BotControl::turn(int nAngle)
{
	bool rtn = 0;
	
	const double left45 = 0.18;
	const double right45 = 0.27;

	const double left90 = 0.21;
	const double right90 = 0.28;

	string s = "";

	if (nAngle > 0)
	{	
		if (nAngle > 45)
		{
			s = setStep(nAngle, left90);
		}
		else
		{
			if (nAngle <= 45)
			{
				s = setStep(nAngle, left45);
			}

		}

		string command = "left 1 5 " + s + " ";

		string cmd = getMovementCMD(command);
		string read = sendCMD(cmd);

		if (read != "err")
		{
			rtn = 1;
		}
	}
	else
	{
		if (nAngle < 0)
		{	
			nAngle = nAngle * -1;
			if (nAngle > 45)
			{
				s = setStep(nAngle, right90);
			}
			else
			{
				if (nAngle <= 45)
				{
					s = setStep(nAngle, right45);
				}
			}
			
			
			string command = "right 1 5 " + s + " ";

			string cmd = getMovementCMD(command);
			string read = sendCMD(cmd);

			if (read != "err")
			{
				rtn = 1;
			}
		}
	}	

	return rtn;
}

int BotControl::getDistance()
{
	string cmd = getSensorCMD("3");

	string read = "";
	read = sendCMD(cmd);
	
	int rtn = -1;

	if (read != "err")
	{
		read = read.substr(0, read.find(","));			
		rtn	= atoi(read.c_str());
	}
	
	return rtn;
}

BotControl::~BotControl(void)
{
}

wstring BotControl::getPortConnect(map<wstring,wstring> ports)
{
	map<wstring,wstring>::iterator it;
	
	for(it = ports.begin(); it != ports.end(); ++it)
	{
		int pos = it->second.find(L"BthModem0");
		if(pos != -1)
		{
			wstring rtn = it->first;
			return rtn;
		}
	}

	return L"Error";
}

wstring BotControl::findeCOMPort()
{
	map<wstring,wstring> portMap;
	
	int r = 0;
	HKEY hkey = NULL;

	r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM\\"), 0, KEY_READ, &hkey);
	if (r != ERROR_SUCCESS) 
	{
		wstring rtn = L"Err";
		return rtn;
	}
  
	unsigned long CountValues = 0, MaxValueNameLen = 0, MaxValueLen = 0;

	RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &CountValues, &MaxValueNameLen, &MaxValueLen, NULL, NULL);
	++MaxValueNameLen;

	TCHAR *bufferName = NULL, *bufferData = NULL;
	bufferName = (TCHAR*)malloc(MaxValueNameLen * sizeof(TCHAR));
	if (!bufferName)
	{
		RegCloseKey(hkey);
		wstring rtn = L"Err";

		return rtn;
	}
	bufferData = (TCHAR*)malloc((MaxValueLen + 1)*sizeof(TCHAR));
	if (!bufferData) 
	{ 
		free(bufferName); 
		RegCloseKey(hkey);

		wstring rtn = L"Err";
		return rtn;
	}
  
	unsigned long NameLen, type, DataLen;

	for (unsigned int i = 0; i < CountValues; i++)
	{
		NameLen = MaxValueNameLen;
		DataLen = MaxValueLen;
		r = RegEnumValue(hkey, i, bufferName, &NameLen, NULL, &type, (LPBYTE)bufferData, &DataLen);
		if ((r != ERROR_SUCCESS) || (type != REG_SZ))
		{
			continue;    
		}

		portMap[bufferData] = bufferName;
	}

	free(bufferName);
	free(bufferData);

	RegCloseKey(hkey);

	wstring rtn = getPortConnect(portMap);

	return rtn;
}

string BotControl::setStep(int nAngle, const double Konst)
{
	string rtn = "";
	double step = (double)nAngle;

	step = step/Konst;
	ostringstream st;
	st << (int)step;

	rtn = st.str();

	return rtn;
}

string BotControl::getMovementCMD(string com)
{
	string command = "{\"cmd\":\"\",\"params\":{\"spd\":0,\"acc\":0,\"dis\":0}}";

	rapidjson::Document doc;
	bool isParse = (doc.Parse(command.c_str())).HasParseError();
	
	if ((doc.Parse(command.c_str())).HasParseError())
	{
		cout << "Document Parse Error" << endl;
		return "1";
	}
	assert(doc.IsObject());
//___	
	string str= "";
	int pos = com.find_first_of(" ");
	str = com.substr(0 ,pos);
//___
	com = com.substr(pos + 1 , com.length());
	
	rapidjson::Value::MemberIterator& sshCMD = doc.FindMember("cmd");
	sshCMD->value.SetString(str.c_str(), doc.GetAllocator());

	for (rapidjson::Value::MemberIterator& m = doc["params"].MemberBegin(); m != doc["params"].MemberEnd(); ++m) 
	{
		pos = com.find_first_of(" ");
		str = com.substr(0 ,pos);
		com = com.substr(pos+1 , com.length());
	
		m->value.SetInt(atoi(str.c_str()));
	}
	
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	string theJSON = buffer.GetString();

	return  theJSON;

}

string BotControl::getSensorCMD(string com)
{
	string command = "{\"cmd\":\"sensor\",\"count\":\"0\"}";

	rapidjson::Document doc;
	bool isParse = (doc.Parse(command.c_str())).HasParseError();
	if ((doc.Parse(command.c_str())).HasParseError())
	{
		cout << "Document Parse Error" << endl;
		return "1";
	}
	assert(doc.IsObject());
				
	int count = 0;
	count = atoi(com.c_str());

	rapidjson::Value::MemberIterator& sshSensor = doc.FindMember("count");
	sshSensor->value.SetInt(count);
				
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	string theJSON = buffer.GetString();
			
	return theJSON;
}

string BotControl::getModeCMD (int mode)
{
	string command = "";
	switch(mode)
	{
		case 1:
			{
				command = "{\"cmd\":\"mode\",\"MS1\":0,\"MS2\":0,\"MS3\":0}";
				break;
			}	
		case 2:
			{
				command = "{\"cmd\":\"mode\",\"MS1\":1,\"MS2\":0,\"MS3\":0}";
				break;
			}
		case 3:
			{
				command = "{\"cmd\":\"mode\",\"MS1\":0,\"MS2\":1,\"MS3\":0}";
				break;
			}
		case 4:
			{
				command = "{\"cmd\":\"mode\",\"MS1\":1,\"MS2\":1,\"MS3\":0}";
				break;
			}
		case 5:
			{
				command = "{\"cmd\":\"mode\",\"MS1\":1,\"MS2\":1,\"MS3\":1}";
				break;
			}
		}

	return command;

}

DWORD WINAPI BotControl::readCOM(LPVOID pParam)
{
	ThParam* param = (ThParam*)pParam;

	COMSTAT comstat; 
	DWORD btr = 0, temp = 0, mask = 0, signal = 0;

	param->overlappedR.OffsetHigh = 0;
    param->overlappedR.Offset = 0;
	param->overlappedR.hEvent = CreateEvent(NULL, false, true, NULL); 

	SetCommMask(param->hSerial, EV_RXCHAR); 

    while (true)
    {
		WaitCommEvent(param->hSerial, &mask, &param->overlappedR);
			
		if(GetOverlappedResult(param->hSerial, &param->overlappedR, &temp, true)) 
		{	
			if((mask & EV_RXCHAR)!=0)
			{
				WaitForSingleObject(param->hSerial,100);
				ClearCommError(param->hSerial, &temp, &comstat);
				btr = comstat.cbInQue;
					
				while(btr > 0)
				{					
					char rBuffer[30] = {0};
						
					ReadFile(param->hSerial, rBuffer, comstat.cbInQue, &temp, &param->overlappedR);
					    
					param->readMSG = param->readMSG + rBuffer;

					ClearCommError(param->hSerial, &temp, &comstat);
					btr = comstat.cbInQue; 
						
					if(btr == 0)
					{
						SetEvent(param->hEvent1);
					}
				}				
			}
		}	 
	}

	return 0;
}

string BotControl::sendCMD(string cmd)
{
	Param->overlappedW.OffsetHigh = 0;
    Param->overlappedW.Offset = 0;
	Param->overlappedW.hEvent = CreateEvent(NULL, true, true, NULL);

	char command[60]; 
	strcpy_s(command, cmd.c_str());
	
	DWORD dwSize = sizeof(command);
	DWORD dwBytesWritten;

	PurgeComm(Param->hSerial,PURGE_RXCLEAR);
	PurgeComm(Param->hSerial,PURGE_TXCLEAR);

	Param->hEvent1=CreateEvent( NULL, false, false, NULL ); 
	HANDLE thread01 = CreateThread (NULL, 0, &BotControl::readCOM, Param, 0, NULL);
	
	BOOL iWrite = WriteFile (Param->hSerial,command,dwSize,&dwBytesWritten , &Param->overlappedW);
	CloseHandle(Param->overlappedW.hEvent);
	
	WaitForSingleObject (Param->hEvent1, INFINITE);
	
	string  str = Param->readMSG;
	Param->readMSG = "";

	CloseHandle(thread01);

	Sleep(500);

	return str;	
}