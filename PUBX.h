#ifndef _PUBX_H_
#define _PUBX_H_

#include <Arduino.h>
// #include "thread"
// #include "mutex"

// std::mutex mtx;

class PUBX
{
    public:

        PUBX(void);

        PUBX(void* buf, uint8_t len);

        ~PUBX();

        static unsigned int parseUnsignedInt(const char *s, uint8_t len);

        static long parseFloat(const char* s, uint8_t log10Multiplier, const char** eptr);

        static long parseDegreeMinute(const char* s, uint8_t degWidth, const char** eptr);

        static const char* parseChar(const char* s, char* result = nullptr, uint8_t len);

        static const char* skipField(const char* s);

        static bool testCheckSum(const char* s);

        static const char* generateChecksum(const char* s, char* checksum);

        void setBuffer(void* buf, uint8_t len);

        void clear(void);

        bool process(char c);

        long getAltitude(void) const
        {
            return _altitude;
        }

        long getLatitude(void) const
        {
            return _latitude;
        }

        long getLongitude(void) const
        {
            return _longitude;
        }

        uint8_t getSecond(void) const
        {
            return _second;
        }

        uint8_t getMinute(void) const
        {
            return _minute;
        }

        uint8_t getHour(void) const
        {
            return _hour;
        }

        uint8_t getDay(void) const
        {
            return _day;
        }

        uint8_t getHundredth(void) const
        {
            return _hundredths;
        }

        uint8_t getMonth(void) const
        {
            return _month;
        }

        uint16_t getYear(void) const
        {
            return _year;
        }

        uint8_t getNumSat(void) const
        {
            return _numSat;
        }

        uint8_t gethAcc(void) const
        {
            return _hAcc;
        }

        uint8_t getvAcc(void) const
        {
            return _vAcc;
        }


        /**
         * @brief Get the vertical dilution of precision (VDOP), in tenths
        * @details A VDOP value of 1.1 is returned as `11`
        * @return uint8_t
        */
        uint8_t getVDOP(void) const 
        {
            return _vdop;
        }

        /**
         * @brief Get the position dilution of precision (PDOP), in tenths
        * @details A PDOP value of 1.1 is returned as `11`
        * @return uint8_t
        */
        uint8_t getPDOP(void) const
        {
            return _pdop;
        }


    protected:

        static inline bool EndOfField(char c)
        {
            return ( (c == '\0') || (c == '\n') || (c == '\r') || (c == '*') );
        }

        const char* parseTime(const char* s);

        const char* parseDate(const char* s);

        bool processPUBX(const char* s);

        bool processGNS(const char* s);

        bool processGSA(const char* s);

        bool processGLL(const char* s);

        bool processGGA(const char *s);

        bool processRMC(const char* s);



    private:

        uint8_t _bufferLen;
        char* _buffer;
	    char *_ptr;

        char _messageID[6];
        uint8_t mgsID;
        long _latitude, _longitude; // In millionths of a degree
	    long _altitude;             // In millimetres
        uint16_t _year;
        uint8_t _month, _day, _hour, _minute, _second, _hundredths;
        uint8_t _numSat;
        long _speed, _course;
        char navStat[2];
        int _isValid;
        bool _altitudeValid;

        uint8_t _hdop;
        uint8_t _vdop;
        uint8_t _pdop;
        uint8_t _tdop;

        uint8_t _hAcc;
        uint8_t _vAcc;
};


#endif  /* _PUBX_H_ */