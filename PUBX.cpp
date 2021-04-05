#include "PUBX.h"




PUBX::PUBX(void):
	_messageID{0}
{
	setBuffer(nullptr, 0);
	clear();
}


PUBX::PUBX(void* buf, uint8_t len):
	_messageID{0}
{
	setBuffer(buf, len);
	clear();
}


void PUBX::setBuffer(void* buf, uint8_t len)
{
	_bufferLen = len;
	_buffer = (char*)buf;
	_ptr = _buffer;
	if (_bufferLen)
	{
		*_ptr = '\0';
		_buffer[_bufferLen - 1] = '\0';
	}
}


void PUBX::clear(void)
{
	_numSat = 0;
	_latitude = 999000000L;
	_longitude = 999000000L;
	_altitude = _speed = _course = LONG_MIN;
	_year = _month = _day = 0;
	_hour = _minute = _second = 99;
	_hundredths = 0;
	
}


static long exp10(uint8_t a)
{
	long r = 1;
	while (a--)
    {
		r *= 10;
    }
    return r;
}


static char toHex(uint8_t a)
{
	if (a >= 10)
		return (a + 'A' - 10);
	else
		return (a + '0');

}


const char* PUBX::skipField(const char* s)
{
	if (s == nullptr)
		return nullptr;

	while (!EndOfField(*s))     //as long as the sentence doesnt end
    {
		if (*s == ',')          //see ","
        {
			if (EndOfField(*++s))   //check the char after the ","
			{
                break;      //if end point then end immediately
            }
            else                    //else point to that char
            {
				return s;
            }
        }
		++s;    //step forward
	}
	return nullptr; // End of string or valid sentence
}


unsigned int PUBX::parseUnsignedInt(const char *s, uint8_t len)
{
	int r = 0;
	while (len--)
    {
		r = 10 * r + *s++ - '0';
    }
	return r;
}


long PUBX::parseFloat(const char* s, uint8_t log10Multiplier, const char** eptr)
{
	int8_t neg = 1;
	long r = 0;

	while (isspace(*s))     //skip all blank space
    {
		++s;        //step forward
    }

	if (*s == '-')      //minus sign
    {
		neg = -1;       //mul (-1) at the end
		++s;            //step forward
	}
	else if (*s == '+')
    {
		++s;            //just step forward
    }

	while (isdigit(*s))     //integer data
    {
		r = 10*r + *s++ - '0';
    }

	r *= exp10(log10Multiplier);    //mul exp 10 to get rid of the float "."

	if (*s == '.')      //fraction data
    {
		++s;        //step forward over the "."
		long frac = 0;

		while (isdigit(*s) && log10Multiplier)      //whatever run out first (data or how long we need the data)
        {
			frac = 10 * frac + *s++ -'0';
			--log10Multiplier;
		}

		frac *= exp10(log10Multiplier);
		r += frac;
	}
	
    r *= neg; // Include effect of any minus sign

	if (eptr)
		*eptr = skipField(s);   //point to the next field

	return r;   //return the float
}


long PUBX::parseDegreeMinute(const char* s, uint8_t degWidth, const char** eptr)
{
	if (*s == ',')      //blank data
    {
		if (eptr)
			*eptr = skipField(s);       //point to the next field
		return 0;
	}
	long r = parseUnsignedInt(s, degWidth) * 1000000L;      //get degree
	s += degWidth;
	r += parseFloat(s, 6, eptr) / 60;       //60 minutes to 1 degree
	return r;
}


const char* PUBX::parseChar(const char* s, char* result = nullptr, uint8_t len)
{
	if (s == nullptr)
		return nullptr;

	int i = 0;
	while (*s != ',' && !EndOfField(*s))
	{
		if (result && i++ < len)
			*result++ = *s;
		++s;
	}
	
	if (result && i < len)
		*result = '\0'; 	// Terminate unless too long

	if (*s == ',')
		return ++s; 		//point to next field
	else
		return nullptr;
}


bool PUBX::testCheckSum(const char* s)
{
	char CheckSum[2];
	const char* p = generateChecksum(s, CheckSum);

	return ( (*p == '*') && (p[1] == CheckSum[0]) && (p[2] == CheckSum[1]) );
}


const char* PUBX::generateChecksum(const char* s, char* checksum)
{
	uint8_t c = 0;
	
	if (*s == '$')		// Initial $ is omitted from checksum, if present ignore it
	{
		++s;
	}

	while (*s != '\0' && *s != '*')
	{
		c ^= *s++;		//XOR
	}

	if (checksum)
	{
		checksum[0] = toHex(c / 16);
		checksum[1] = toHex(c % 16);
	}

	return s;
}


const char* PUBX::parseTime(const char* s)
{
	if (*s == ',')
		return skipField(s);

	_hour = parseUnsignedInt(s, 2);
	_minute = parseUnsignedInt(s + 2, 2);
	_second = parseUnsignedInt(s + 4, 2);
	_hundredths = parseUnsignedInt(s + 7, 2);

	return skipField(s + 9);
}


const char* PUBX::parseDate(const char* s)
{
	if (*s == ',')
		return skipField(s);

	_day = parseUnsignedInt(s, 2);
	_month = parseUnsignedInt(s + 2, 2);
	_year = parseUnsignedInt(s + 4, 2) + 2000;

	return skipField(s + 6);
}


bool PUBX::process(char c)
{
	if (_buffer == nullptr || _bufferLen == 0)
	{
		return false;
	}

	if (c == '\0' || c == '\n' || c == '\r')
	{
		// Terminate buffer then reset pointer
		*_ptr = '\0';
		_ptr = _buffer;

		if (*_buffer == '$' && testCheckSum(_buffer))
		{
			// Valid message
			const char* data;

			data = parseChar(&_buffer[1], &_messageID[0], sizeof(_messageID));		//point to data fields, get UBX protocol header

			if ((data != nullptr) && (strcmp(&_messageID[0], "PUBX") == 0))			//check data pointed, correct UBX protocol header
			{
				return processPUBX(data);
			}
		
		}

		return (*_buffer != '\0');		//return true for a complete, non-empty, sentence (even if not a valid one)
	}
	else
	{
		*_ptr = c;
		if (_ptr < &_buffer[_bufferLen - 1])
		{
			++_ptr;
		}
	}

	return false;
}


bool PUBX::processPUBX(const char* s)
{
	mgsID = parseUnsignedInt(s, 2);

	s = skipField(s);
	if (s == nullptr)
		return false;
	
	s = parseTime(s);
	if (s == nullptr)
		return false;

	_latitude = parseDegreeMinute(s, 2, &s);
	if (s == nullptr)
		return false;

	if (*s == ',')		//check NS
		++s;
	else
	{
		if (*s == 'S')
			_latitude *= -1;
		s += 2; // Skip (N or S) and comma
	}

	_longitude = parseDegreeMinute(s, 3, &s);
	if (s == nullptr)
		return false;
	
	if (*s == ',')		//check EW
		++s;
	else
	{
		if (*s == 'W')
			_longitude *= -1;
		s += 2; // Skip (E or W) and comma
	}

	_altitude = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	s = parseChar(s, &navStat[0], sizeof(navStat));
	if (s == nullptr)
		return false;

	_hAcc = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	_vAcc = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	_speed = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	_course = parseFloat(s, 2, &s);
	if (s == nullptr)
		return false;

	s = skipField(s);
	if (s == nullptr)
		return false;

	s = skipField(s);
	if (s == nullptr)
		return false;

	_hdop = parseFloat(s, 2, &s);
	if (s == nullptr)
		return false;

	_vdop = parseFloat(s, 2, &s);
	if (s == nullptr)
		return false;

	_tdop = parseFloat(s, 2, &s);
	if (s == nullptr)
		return false;

	_numSat = parseUnsignedInt(s, 2);
	s = skipField(s);
	if (s == nullptr)
		return false;

	return true;	//Gathered all data!

}


bool PUBX::processGNS(const char* s)
{

	s = parseTime(s);
	if (s == nullptr)
	{
		return false;
	}

	_latitude = parseDegreeMinute(s, 2, &s);
	if (s == nullptr)
	{
		return false;
	}

	if (*s == ',')
	{
		++s;
	}
	else
	{
		if (*s == 'S')
		{
			_latitude *= -1;
		}
		s += 2; // Skip (N or S) and comma
	}

	_longitude = parseDegreeMinute(s, 3, &s);
	if (s == nullptr)
	{
		return false;
	}
	
	if (*s == ',')
	{
		++s;
	}
	else
	{
		if (*s == 'W')
		{
			_longitude *= -1;
		}
		s += 2; // Skip (E or W) and comma
	}

	s = skipField(s);

	_numSat = parseFloat(s, 0, &s);
	if (s == nullptr)
	{
		return false;
	}
	
	_hdop = parseFloat(s, 1, &s);
	if (s == nullptr)
	{
		return false;
	}
	
	_altitude = parseFloat(s, 3, &s);
	if (s == nullptr)
	{
		return false;
	}

	_altitudeValid = true;

	//that is all we r interested in

	return true;
}


bool PUBX::processGSA(const char* s)
{
	int i = 0;
	while(i < 2)
	{
		s = skipField(s);
		i++;
	}
	
	i = 0;
	unsigned int svid[12];
	while(i < 12)
	{
		svid[i] = parseUnsignedInt(s, 2);
		if (s == nullptr)
		{
			return false;
		}
		i++;
	}

	_pdop = parseFloat(s, 2, &s);
	if (s == nullptr)
	{
		return false;
	}

	_hdop = parseFloat(s, 2, &s);
	if (s == nullptr)
	{
		return false;
	}

	_vdop = parseFloat(s, 2, &s);
	if (s == nullptr)
	{
		return false;
	}

	//that is all we r interested in	

	return true;
}


bool PUBX::processGLL(const char* s)
{

	_latitude = parseDegreeMinute(s, 2, &s);
	if (s == nullptr)
	{
		return false;
	}

	if (*s == ',')
	{
		++s;
	}
	else
	{
		if (*s == 'S')
		{
			_latitude *= -1;
		}
		s += 2; // Skip (N or S) and comma
	}

	_longitude = parseDegreeMinute(s, 3, &s);
	if (s == nullptr)
	{
		return false;
	}
	
	if (*s == ',')
	{
		++s;
	}
	else
	{
		if (*s == 'W')
		{
			_longitude *= -1;
		}
		s += 2; // Skip (E or W) and comma
	}

	s = parseTime(s);
	if (s == nullptr)
	{
		return false;
	}
	
	//that is all we care about here

	return true;
}


bool PUBX::processGGA(const char *s)
{

	s = parseTime(s);
	if (s == nullptr)
		return false;

	_latitude = parseDegreeMinute(s, 2, &s);
	if (s == nullptr)
		return false;

	if (*s == ',')
		++s;
	else {
		if (*s == 'S')
			_latitude *= -1;
		s += 2; // Skip N/S and comma
	}

	_longitude = parseDegreeMinute(s, 3, &s);
	if (s == nullptr)
		return false;
	if (*s == ',')
		++s;
	else {
		if (*s == 'W')
			_longitude *= -1;
		s += 2; // Skip E/W and comma
	}

	_isValid = (*s >= '1' && *s <= '5');
	s += 2; // Skip position fix flag and comma

	_numSat = parseFloat(s, 0, &s);
	if (s == nullptr)
		return false;

	_hdop = parseFloat(s, 1, &s);
	if (s == nullptr)
		return false;

	_altitude = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	_altitudeValid = true;
	// That's all we care about
	return true;
}


bool PUBX::processRMC(const char* s)
{

	s = parseTime(s);
	if (s == nullptr)
		return false;

	_isValid = (*s == 'A');
	s += 2; // Skip validity and comma

	_latitude = parseDegreeMinute(s, 2, &s);
	if (s == nullptr)
		return false;

	if (*s == ',')
		++s;
	else {
		if (*s == 'S')
			_latitude *= -1;
		s += 2; // Skip N/S and comma
	}

	_longitude = parseDegreeMinute(s, 3, &s);
	if (s == nullptr)
		return false;
	
	if (*s == ',')
		++s;
	else {
		if (*s == 'W')
			_longitude *= -1;
		s += 2; // Skip E/W and comma
	}

	_speed = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	_course = parseFloat(s, 3, &s);
	if (s == nullptr)
		return false;

	s = parseDate(s);
	// That's all we care about
	return true;
}

