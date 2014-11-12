/*
    This file is part of AutoQuad.

    AutoQuad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AutoQuad is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with AutoQuad.  If not, see <http://www.gnu.org/licenses/>.

    Copyright © 2011-2014  Bill Nesbitt
*/

#include "logger.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

loggerFields_t *loggerFields;
int loggerNumFields;
int loggerPacketSize;

void loggerChecksumError(const char *s) {
	fprintf(stderr, "logger: checksum error in '%s' packet\n", s);
}

void loggerDecodePacket(char *buf, loggerRecord_t *r) {
	int i;
	unsigned char fieldId;

	for (i = 0; i < loggerNumFields; i++) {
		fieldId = loggerFields[i].fieldId;
		// store some fields in arrays, for convenience
		switch (fieldId) {
			case LOG_VOLTAGE0:
			case LOG_VOLTAGE1:
			case LOG_VOLTAGE2:
			case LOG_VOLTAGE3:
			case LOG_VOLTAGE4:
			case LOG_VOLTAGE5:
			case LOG_VOLTAGE6:
			case LOG_VOLTAGE7:
			case LOG_VOLTAGE8:
			case LOG_VOLTAGE9:
			case LOG_VOLTAGE10:
			case LOG_VOLTAGE11:
			case LOG_VOLTAGE12:
			case LOG_VOLTAGE13:
			case LOG_VOLTAGE14:
				r->voltages[fieldId-LOG_VOLTAGE0] = *(float *)buf;
				break;
			case LOG_UKF_Q1:
			case LOG_UKF_Q2:
			case LOG_UKF_Q3:
			case LOG_UKF_Q4:
				r->quat[fieldId-LOG_UKF_Q1] = *(float *)buf;
				break;
			case LOG_MOT_MOTOR0:
			case LOG_MOT_MOTOR1:
			case LOG_MOT_MOTOR2:
			case LOG_MOT_MOTOR3:
			case LOG_MOT_MOTOR4:
			case LOG_MOT_MOTOR5:
			case LOG_MOT_MOTOR6:
			case LOG_MOT_MOTOR7:
			case LOG_MOT_MOTOR8:
			case LOG_MOT_MOTOR9:
			case LOG_MOT_MOTOR10:
			case LOG_MOT_MOTOR11:
			case LOG_MOT_MOTOR12:
			case LOG_MOT_MOTOR13:
				r->motors[fieldId-LOG_MOT_MOTOR0] = *(uint16_t *)buf;
				break;
			case LOG_RADIO_CHANNEL0:
			case LOG_RADIO_CHANNEL1:
			case LOG_RADIO_CHANNEL2:
			case LOG_RADIO_CHANNEL3:
			case LOG_RADIO_CHANNEL4:
			case LOG_RADIO_CHANNEL5:
			case LOG_RADIO_CHANNEL6:
			case LOG_RADIO_CHANNEL7:
			case LOG_RADIO_CHANNEL8:
			case LOG_RADIO_CHANNEL9:
			case LOG_RADIO_CHANNEL10:
			case LOG_RADIO_CHANNEL11:
			case LOG_RADIO_CHANNEL12:
			case LOG_RADIO_CHANNEL13:
			case LOG_RADIO_CHANNEL14:
			case LOG_RADIO_CHANNEL15:
			case LOG_RADIO_CHANNEL16:
			case LOG_RADIO_CHANNEL17:
				r->radioChannels[fieldId-LOG_RADIO_CHANNEL0] = *(int16_t *)buf;
				break;
		}

		switch (loggerFields[i].fieldType) {
			case LOG_TYPE_DOUBLE:
				r->data[fieldId] = *(double *)buf;
				buf += 8;
				break;
			case LOG_TYPE_FLOAT:
				r->data[fieldId] = *(float *)buf;
				buf += 4;
				break;
			case LOG_TYPE_U32:
				r->data[fieldId] = *(uint32_t *)buf;
				buf += 4;
				break;
			case LOG_TYPE_S32:
				r->data[fieldId] = *(int32_t *)buf;
				buf += 4;
				break;
			case LOG_TYPE_U16:
				r->data[fieldId] = *(uint16_t *)buf;
				buf += 2;
				break;
			case LOG_TYPE_S16:
				r->data[fieldId] = *(int16_t *)buf;
				buf += 2;
				break;
			case LOG_TYPE_U8:
				r->data[fieldId] = *(uint8_t *)buf;
				buf += 1;
				break;
			case LOG_TYPE_S8:
				r->data[fieldId] = *(int8_t *)buf;
				buf += 1;
				break;
		}
	}
}

int loggerReadEntryM(FILE *fp, loggerRecord_t *r) {
	char buf[1024];
	unsigned char ckA, ckB;
	int i;

	if (loggerPacketSize > 0 && fread(buf, loggerPacketSize, 1, fp) == 1) {
		// calc checksum
		ckA = ckB = 0;
		for (i = 0; i < loggerPacketSize; i++) {
			ckA += buf[i];
			ckB += ckA;
		}

		if (fgetc(fp) == ckA && fgetc(fp) == ckB) {
			loggerDecodePacket(buf, r);

			return 1;
		}

		loggerChecksumError("M");
	}

	return 0;
}

int loggerReadEntryH(FILE *fp) {
	char buf[1024];
	unsigned char ckA, ckB;
	int numFields;
	int i;

	numFields = fgetc(fp);

	if (fread(buf, numFields * sizeof(loggerFields_t), 1, fp) == 1) {
		// calc checksum
		ckA = ckB = numFields;
		for (i = 0; i < numFields * sizeof(loggerFields_t); i++) {
			ckA += buf[i];
			ckB += ckA;
		}

		if (fgetc(fp) == ckA && fgetc(fp) == ckB) {
			loggerFields = (loggerFields_t *)realloc(loggerFields, numFields * sizeof(loggerFields_t));
			memcpy(loggerFields, buf, numFields * sizeof(loggerFields_t));
			loggerNumFields = numFields;

			loggerPacketSize = 0;
			for (i = 0; i < numFields; i++) {
				switch (loggerFields[i].fieldType) {
					case LOG_TYPE_DOUBLE:
						loggerPacketSize += 8;
						break;
					case LOG_TYPE_FLOAT:
					case LOG_TYPE_U32:
					case LOG_TYPE_S32:
						loggerPacketSize += 4;
						break;
					case LOG_TYPE_U16:
					case LOG_TYPE_S16:
						loggerPacketSize += 2;
						break;
					case LOG_TYPE_U8:
					case LOG_TYPE_S8:
						loggerPacketSize += 1;
						break;
				}
			}

			return 1;
		}
		else {
			loggerChecksumError("H");
		}
	}

	return 0;
}

int loggerReadEntryL(FILE *fp, loggerRecord_t *r) {
	char *buf = (char *)r;
	char ckA, ckB;
	int i;

	if (fread(buf, sizeof(loggerRecord_t), 1, fp) == 1) {
		// calc checksum
		ckA = ckB = 0;
		for (i = 0; i < sizeof(loggerRecord_t) - 2; i++) {
			ckA += buf[i];
			ckB += ckA;
		}

		if (r->ckA == ckA && r->ckB == ckB) {
			return 1;
		}
		else {
			loggerChecksumError("L");
			return 0;
		}
	}
	return 0;
}

int loggerReadEntry(FILE *fp, loggerRecord_t *r) {
	int c = 0;

	loggerTop:

	if (c != EOF) {
		if ((c = fgetc(fp)) != 'A')
			goto loggerTop;
		if ((c = fgetc(fp)) != 'q')
			goto loggerTop;

		c = fgetc(fp);

		if (c == 'L') {
			if (loggerReadEntryL(fp, r) == 0)
				goto loggerTop;
			else
				return 1;
		}
		else if (c == 'H') {
			loggerReadEntryH(fp);
			goto loggerTop;
		}
		else if (c == 'M') {
			if (loggerReadEntryM(fp, r) == 0)
				goto loggerTop;
			else
				return 1;
		}
		else {
//			fprintf(stderr, "logger: Unknown record type '%d'\n", c);
			goto loggerTop;
		}

	}

	return EOF;
}

// allocates memory and reads an entire log
int loggerReadLog(const char *fname, loggerRecord_t **l) {
	loggerRecord_t buf;
	FILE *fp;
	int n = 0;
	int i;

	*l = NULL;

#if defined (__WIN32__)
	fp = fopen(fname, "rb");
#else
	fp = fopen(fname, "r");
#endif
	if (fp == NULL) {
		fprintf(stderr, "logger: cannot open log file '%s'\n", fname);
	}
	else {
		loggerPacketSize = 0;
		loggerNumFields = 0;

		// force header read
		loggerReadEntry(fp, &buf);
		rewind(fp);

		while (loggerReadEntry(fp, &buf) != EOF)
			n++;

		*l = (loggerRecord_t *)calloc(n, sizeof(loggerRecord_t));

		rewind(fp);

		for (i = 0; i < n; i++)
			loggerReadEntry(fp, &(*l)[i]);

		fclose(fp);
	}

	return n;
}

int loggerRecordSize(void) {
	if (loggerPacketSize)
		return loggerPacketSize + 2 + 3;
	else
		return (sizeof(loggerRecord_t));
}

void loggerFree(loggerRecord_t *l) {
	if (l) {
		free(l);
		l = NULL;
	}

	if (loggerFields) {
		free(loggerFields);
		loggerPacketSize = 0;
		loggerNumFields = 0;
	}
}
