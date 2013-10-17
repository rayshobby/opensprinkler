/*
	tinyFAT.cpp - Arduino library support FAT16 on SD cards
	Copyright (C)2010-2011 Henning Karlsen. All right reserved

	You can find the latest version of the library at 
	http://www.henningkarlsen.com/electronics

	This library has been made to easily use SD card with the Arduino.

	If you make any modifications or improvements to the code, I would appreciate
	that you share the code with me so that I might include it in the next release.
	I can be contacted through http://www.henningkarlsen.com/electronics/contact.php

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "tinyFAT.h"
#include <mmc.h>

/* Public */

tinyFAT::tinyFAT()
{
	_inited=false;
}

byte tinyFAT::initFAT(byte speed)
{
	mmc::initialize(speed);
// Read MBR
	if (RES_OK == mmc::readSector(buffer, 0))
	{
		if ((buffer[0x01FE]==0x55) && (buffer[0x01FF]==0xAA))
		{
			MBR.part1Type=buffer[450];
			MBR.part1Start = uint16_t(buffer[454])+(uint16_t(buffer[455])<<8)+(uint32_t(buffer[456])<<16)+(uint32_t(buffer[457])<<24);
			MBR.part1Size = uint16_t(buffer[458])+(uint16_t(buffer[459])<<8)+(uint32_t(buffer[460])<<16)+(uint32_t(buffer[461])<<24);
		}
		else
		{
			return ERROR_MBR_SIGNATURE;
		}
	}
	else
		return ERROR_MBR_READ_ERROR;

	if ((MBR.part1Type!=0x04) && (MBR.part1Type!=0x06) && (MBR.part1Type!=0x86))
	{
		return ERROR_MBR_INVALID_FS;
	}

// Read Boot Sector
	if (RES_OK == mmc::readSector(buffer, MBR.part1Start))
    {
		if ((buffer[0x01FE]==0x55) && (buffer[0x01FF]==0xAA))
		{
			BS.sectorsPerCluster = buffer[0x0D];
			BS.reservedSectors = uint16_t(buffer[0x0E])+(uint16_t(buffer[0x0F])<<8);
			BS.fatCopies = buffer[0x10];
			BS.rootDirectoryEntries = uint16_t(buffer[0x11])+(uint16_t(buffer[0x12])<<8);
			BS.totalFilesystemSectors = uint16_t(buffer[0x13])+(uint16_t(buffer[0x14])<<8);
			if (BS.totalFilesystemSectors==0)
				BS.totalFilesystemSectors = uint16_t(buffer[0x20])+(uint16_t(buffer[0x21])<<8)+(uint32_t(buffer[0x22])<<16)+(uint32_t(buffer[0x23])<<24);
			BS.sectorsPerFAT = uint16_t(buffer[0x16])+(uint16_t(buffer[0x17])<<8);
			BS.hiddenSectors = uint16_t(buffer[0x1C])+(uint16_t(buffer[0x1D])<<8)+(uint32_t(buffer[0x1E])<<16)+(uint32_t(buffer[0x1F])<<24);
			BS.partitionSerialNum = uint16_t(buffer[0x27])+(uint16_t(buffer[0x28])<<8)+(uint32_t(buffer[0x29])<<16)+(uint32_t(buffer[0x2A])<<24);
			firstDirSector = MBR.part1Start + BS.reservedSectors + (BS.fatCopies * BS.sectorsPerFAT);
			BS.fat1Start = MBR.part1Start + BS.reservedSectors;
			BS.fat2Start = BS.fat1Start + BS.sectorsPerFAT;
			BS.partitionSize = float((MBR.part1Size*512)/float(1048576));
		}
		else
			return ERROR_BOOTSEC_SIGNATURE;
	}
	else
		return ERROR_BOOTSEC_READ_ERROR;

	_inited=true;
	return 0x00;
}

byte tinyFAT::findFirstFile(_directory_entry *tempDE)
{
	unsigned long currSec = firstDirSector;
	word offset = 0;

	DEcnt=0;
	mmc::readSector(buffer, currSec);

	if (buffer[0]==0x00)
		return ERROR_NO_MORE_FILES;
	else
	{
		while ((buffer[offset + 0x0B] & 0x08) || (buffer[offset + 0x0B] & 0x10) || (buffer[offset]==0xE5))
		{
			offset+=32;
			DEcnt++;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 
			if (buffer[offset]==0x00)
				return ERROR_NO_MORE_FILES;
		}

		for (int i=0; i<8; i++)
		{
			tempDE->filename[i] = buffer[i+offset];
		}
		for (int i=0; i<3; i++)
		{
			tempDE->fileext[i] = buffer[i+0x08+offset];
		}
		tempDE->filename[8] = 0;
		tempDE->fileext[3] = 0;
		tempDE->attributes = buffer[0x0B + offset];
		tempDE->time = uint16_t(buffer[0x0E + offset]) + (uint16_t(buffer[0x0F + offset])<<8);
		tempDE->date = uint16_t(buffer[0x10 + offset]) + (uint16_t(buffer[0x11 + offset])<<8);
		tempDE->startCluster = uint16_t(buffer[0x1A + offset]) + (uint16_t(buffer[0x1B + offset])<<8);
		tempDE->fileSize = uint16_t(buffer[offset + 0x1C]) | (uint16_t(buffer[offset + 0x1D])<<8) | (uint32_t(buffer[offset + 0x1E])<<16) | (uint32_t(buffer[offset + 0x1F])<<24);
		DEcnt++;

		return NO_ERROR;
	}
}

byte tinyFAT::findNextFile(_directory_entry *tempDE)
{
	unsigned long currSec = firstDirSector;
	word offset = DEcnt*32;

	while (offset>=512)
	{
		currSec++;
		offset-=512;
	}

	mmc::readSector(buffer, currSec);

	if (buffer[offset]==0x00)
		return ERROR_NO_MORE_FILES;
	else
	{
		while ((buffer[offset + 0x0B] & 0x08) || (buffer[offset + 0x0B] & 0x10) || (buffer[offset]==0xE5))
		{
			offset+=32;
			DEcnt++;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 
			if (buffer[offset]==0x00)
				return ERROR_NO_MORE_FILES;
		}

		for (int i=0; i<8; i++)
		{
			tempDE->filename[i] = buffer[i+offset];
		}
		for (int i=0; i<3; i++)
		{
			tempDE->fileext[i] = buffer[i+0x08+offset];
		}
		tempDE->filename[8] = 0;
		tempDE->fileext[3] = 0;
		tempDE->attributes = buffer[0x0B + offset];
		tempDE->time = uint16_t(buffer[0x0E + offset]) + (uint16_t(buffer[0x0F + offset])<<8);
		tempDE->date = uint16_t(buffer[0x10 + offset]) + (uint16_t(buffer[0x11 + offset])<<8);
		tempDE->startCluster = uint16_t(buffer[0x1A + offset]) + (uint16_t(buffer[0x1B + offset])<<8);
		tempDE->fileSize = uint16_t(buffer[offset + 0x1C]) | (uint16_t(buffer[offset + 0x1D])<<8) | (uint32_t(buffer[offset + 0x1E])<<16) | (uint32_t(buffer[offset + 0x1F])<<24);
		DEcnt++;

		return NO_ERROR;
	}
}

byte tinyFAT::openFile(char *fn, byte mode)
{
	_directory_entry tmpDE;
	char tmpFN[13];
	byte res;
	int i, j;

	if (currFile.filename[0]!=0x00)
		return ERROR_ANOTHER_FILE_OPEN;

	for (i=0; i<strlen(fn); i++)
		fn[i]=uCase(fn[i]);

	res=findFirstFile(&tmpDE);
	if (res==ERROR_NO_MORE_FILES)
		return ERROR_FILE_NOT_FOUND;
	else
	{
		i=0;
		j=0;
		while ((tmpDE.filename[i]!=0x20) and (i<8))
		{
			tmpFN[i]=tmpDE.filename[i];
			i++;
		}
		tmpFN[i]='.';
		i++;
		while ((tmpDE.fileext[j]!=0x20) and (j<3))
		{
			tmpFN[i]=tmpDE.fileext[j];
			i++;
			j++;
		}
		tmpFN[i]=0x00;
		if (!strcmp(tmpFN,fn))
		{
			for (i=0; i<13; i++)
				currFile.filename[i]=tmpFN[i];
			currFile.currentCluster=tmpDE.startCluster;
			currFile.fileSize=tmpDE.fileSize;
			currFile.currentPos=0;
			currFile.fileMode=mode;
			return NO_ERROR;
		}
		while (res==NO_ERROR)
		{
			res = file.findNextFile(&tmpDE);
			if (res==NO_ERROR)
			{
				i=0;
				j=0;
				while ((tmpDE.filename[i]!=0x20) and (i<8))
				{
					tmpFN[i]=tmpDE.filename[i];
					i++;
				}
				tmpFN[i]='.';
				i++;
				while ((tmpDE.fileext[j]!=0x20) and (j<3))
				{
					tmpFN[i]=tmpDE.fileext[j];
					i++;
					j++;
				}
				tmpFN[i]=0x00;
				if (!strcmp(tmpFN,fn))
				{
					for (i=0; i<13; i++)
						currFile.filename[i]=tmpFN[i];
					currFile.currentCluster=tmpDE.startCluster;
					currFile.fileSize=tmpDE.fileSize;
					currFile.currentPos=0;
					currFile.fileMode=mode;
					return NO_ERROR;
				}
			}
		}


	}
	return ERROR_FILE_NOT_FOUND;
}

uint16_t tinyFAT::readBinary()
{
	uint32_t sec;
	uint8_t status_;

	if (currFile.fileMode==FILEMODE_BINARY)
	{
		if ((currFile.currentPos==0) and (currFile.currentCluster==0))
			return FILE_IS_EMPTY;
		if (((currFile.currentPos % BS.sectorsPerCluster)==0) and (currFile.currentPos>0))
			currFile.currentCluster=findNextCluster(currFile.currentCluster);
		sec=BS.hiddenSectors + (uint32_t)BS.reservedSectors + ((uint32_t)BS.fatCopies*(uint32_t)BS.sectorsPerFAT)+(((uint32_t)BS.rootDirectoryEntries*32)/512)+((uint32_t)currFile.currentCluster-2)*(uint32_t)BS.sectorsPerCluster+((uint32_t)currFile.currentPos % (uint32_t)BS.sectorsPerCluster);
		status_=mmc::readSector(buffer, sec);
		if (status_)
		{
			return status_;
		}
		currFile.currentPos++;
		if ((currFile.currentPos*512)>currFile.fileSize)
		{
				return (currFile.fileSize-((currFile.currentPos-1)*512));
		}
		else
		{
				return 512;
		}
	}
	else
		if (currFile.fileMode==0x00)
			return ERROR_NO_FILE_OPEN;
		else
			return ERROR_WRONG_FILEMODE;
}

uint16_t tinyFAT::readLn(char *st, int bufSize)
{
	uint32_t sec;
	int bufIndex=0;

	for (int i=0; i<=bufSize; i++)
		st[i]=0;

	if (currFile.fileMode==FILEMODE_TEXT_READ)
	{
		if ((currFile.currentPos==0) and (currFile.currentCluster==0))
			return FILE_IS_EMPTY;
		sec=((uint32_t)BS.reservedSectors+((uint32_t)BS.fatCopies*(uint32_t)BS.sectorsPerFAT)+(((uint32_t)BS.rootDirectoryEntries*32)/512)+(((uint32_t)currFile.currentCluster-2)*(uint32_t)BS.sectorsPerCluster)+BS.hiddenSectors)+(((uint32_t)currFile.currentPos/512) % (uint32_t)BS.sectorsPerCluster);
		mmc::readSector(buffer, sec);
		while ((currFile.currentPos<currFile.fileSize) and (buffer[currFile.currentPos % 512]!=10) and (buffer[currFile.currentPos % 512]!=13) and (bufIndex<bufSize))
		{
			st[bufIndex]=buffer[currFile.currentPos % 512];
			bufIndex++;
			currFile.currentPos++;
			if ((currFile.currentPos % 512) == 0)
			{
				sec++;
				if (((currFile.currentPos/512) % BS.sectorsPerCluster)==0)
				{
					currFile.currentCluster=findNextCluster(currFile.currentCluster);
					sec=((uint32_t)BS.reservedSectors+((uint32_t)BS.fatCopies*(uint32_t)BS.sectorsPerFAT)+(((uint32_t)BS.rootDirectoryEntries*32)/512)+(((uint32_t)currFile.currentCluster-2)*(uint32_t)BS.sectorsPerCluster)+BS.hiddenSectors);
				}
				mmc::readSector(buffer, sec);
			}
		}
		if (currFile.currentPos>=currFile.fileSize)
			return EOF;
		else if ((buffer[(currFile.currentPos % 512)]==13) and (buffer[(currFile.currentPos % 512)+1]==10))
		{
			currFile.currentPos+=2;
			return bufIndex;
		}
		else if ((buffer[(currFile.currentPos % 512)]==13) or (buffer[(currFile.currentPos % 512)]==10))
		{
			currFile.currentPos++;
			return bufIndex;
		}
		else
			return BUFFER_OVERFLOW;
	}
	else
		if (currFile.fileMode==0x00)
			return ERROR_NO_FILE_OPEN;
		else
			return ERROR_WRONG_FILEMODE;
}

uint16_t tinyFAT::writeLn(char *st)
{
	unsigned long currSec = firstDirSector;
	uint16_t nextCluster = 0;
	word offset = -32;
	uint32_t sec;
	char tmpFN[13];
	int i, j;
	int bufIndex=0;
	boolean done=false;

	if (currFile.fileMode==FILEMODE_TEXT_WRITE)
	{
		if (currFile.currentCluster==0)
		{
			currFile.currentCluster=findFreeCluster();

			mmc::readSector(buffer, currSec);
			while (!done)
			{
				offset+=32;
				if (offset==512)
				{
				  currSec++;
				  mmc::readSector(buffer, currSec);
				  offset = 0;
				} 

				j=0;
				for (int i=0; i<8; i++)
				{
					if (buffer[i+offset]!=0x20)
					{
						tmpFN[j]=buffer[i+offset];
						j++;
					}
				}
				tmpFN[j]='.';
				j++;
				for (int i=0; i<3; i++)
				{
					if (buffer[i+0x08+offset]!=0x20)
					{
						tmpFN[j]=buffer[i+0x08+offset];
						j++;
					}
				}
				tmpFN[j]=0x00;
				
				if (!strcmp(tmpFN, currFile.filename))
				{
					buffer[offset+0x1A]=currFile.currentCluster & 0xFF;
					buffer[offset+0x1B]=currFile.currentCluster>>8;

					mmc::writeSector(buffer, currSec);

					mmc::readSector(buffer, BS.fat1Start+(currFile.currentCluster>>8));
					buffer[(currFile.currentCluster & 0xFF)*2]=0xFF;
					buffer[((currFile.currentCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat1Start+(currFile.currentCluster>>8));

					mmc::readSector(buffer, BS.fat2Start+(currFile.currentCluster>>8));
					buffer[(currFile.currentCluster & 0xFF)*2]=0xFF;
					buffer[((currFile.currentCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat2Start+(currFile.currentCluster>>8));

					done=true;
				}
			}

		}

		if ((((currFile.fileSize % 512)+strlen(st))<=510) and ((currFile.fileSize % (long(BS.sectorsPerCluster)*512)!=0) or (currFile.fileSize==0)))
		{
			currSec=(BS.reservedSectors+(BS.fatCopies*BS.sectorsPerFAT)+((BS.rootDirectoryEntries*32)/512)+((currFile.currentCluster-2)*BS.sectorsPerCluster)+BS.hiddenSectors)+((currFile.fileSize/512) % BS.sectorsPerCluster);
			mmc::readSector(buffer, currSec);
			for (int i=0; i<strlen(st); i++)
				buffer[(currFile.fileSize%512)+i]=st[i];
			buffer[(currFile.fileSize%512)+strlen(st)]=0x0D;
			buffer[(currFile.fileSize%512)+strlen(st)+1]=0x0A;
			mmc::writeSector(buffer, currSec);
		}
		else
		{
			currSec=(BS.reservedSectors+(BS.fatCopies*BS.sectorsPerFAT)+((BS.rootDirectoryEntries*32)/512)+((currFile.currentCluster-2)*BS.sectorsPerCluster)+BS.hiddenSectors)+((currFile.fileSize/512) % BS.sectorsPerCluster);

			if ((currFile.fileSize%512)!=0)
			{
				mmc::readSector(buffer, currSec);
				for (int i=0; i<(512-(currFile.fileSize%512)); i++)
				{
					buffer[(currFile.fileSize%512)+i]=st[i];
					bufIndex++;
				}
				mmc::writeSector(buffer, currSec);
				currSec++;
			}
			else
				bufIndex=0;

			if (((currSec-(BS.reservedSectors+(BS.fatCopies*BS.sectorsPerFAT)+((BS.rootDirectoryEntries*32)/512)+BS.hiddenSectors)) % BS.sectorsPerCluster)==0)
			{
				nextCluster=findFreeCluster();

				mmc::readSector(buffer, BS.fat1Start+(currFile.currentCluster>>8));
				buffer[(currFile.currentCluster & 0xFF)*2]=nextCluster & 0xFF;
				buffer[((currFile.currentCluster & 0xFF)*2)+1]=nextCluster>>8;
				if ((nextCluster>>8)==(currFile.currentCluster>>8))
				{
					buffer[(nextCluster & 0xFF)*2]=0xFF;
					buffer[((nextCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat1Start+(currFile.currentCluster>>8));
				}
				else
				{
					mmc::writeSector(buffer, BS.fat1Start+(currFile.currentCluster>>8));
					mmc::readSector(buffer, BS.fat1Start+(nextCluster>>8));
					buffer[(nextCluster & 0xFF)*2]=0xFF;
					buffer[((nextCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat1Start+(nextCluster>>8));
				}

				mmc::readSector(buffer, BS.fat2Start+(currFile.currentCluster>>8));
				buffer[(currFile.currentCluster & 0xFF)*2]=nextCluster & 0xFF;
				buffer[((currFile.currentCluster & 0xFF)*2)+1]=nextCluster>>8;
				if ((nextCluster>>8)==(currFile.currentCluster>>8))
				{
					buffer[(nextCluster & 0xFF)*2]=0xFF;
					buffer[((nextCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat2Start+(currFile.currentCluster>>8));
				}
				else
				{
					mmc::writeSector(buffer, BS.fat2Start+(currFile.currentCluster>>8));
					mmc::readSector(buffer, BS.fat2Start+(nextCluster>>8));
					buffer[(nextCluster & 0xFF)*2]=0xFF;
					buffer[((nextCluster & 0xFF)*2)+1]=0xFF;
					mmc::writeSector(buffer, BS.fat2Start+(nextCluster>>8));
				}

				currFile.currentCluster=nextCluster;

				currSec=(BS.reservedSectors+(BS.fatCopies*BS.sectorsPerFAT)+((BS.rootDirectoryEntries*32)/512)+((currFile.currentCluster-2)*BS.sectorsPerCluster)+BS.hiddenSectors);
			}
			mmc::readSector(buffer, currSec);
			for (int i=0; i<strlen(st)-bufIndex; i++)
				buffer[i]=st[i+bufIndex];
			buffer[strlen(st)-bufIndex]=0x0D;
			buffer[strlen(st)-bufIndex+1]=0x0A;
			mmc::writeSector(buffer, currSec);


		}

		currFile.fileSize+=(strlen(st)+2);

		currSec=firstDirSector;
		offset=-32;
		done=false;
		mmc::readSector(buffer, currSec);
		while (!done)
		{
			offset+=32;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 

			j=0;
			for (int i=0; i<8; i++)
			{
				if (buffer[i+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+offset];
					j++;
				}
			}
			tmpFN[j]='.';
			j++;
			for (int i=0; i<3; i++)
			{
				if (buffer[i+0x08+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+0x08+offset];
					j++;
				}
			}
			tmpFN[j]=0x00;

			if (!strcmp(tmpFN, currFile.filename))
			{
				buffer[offset+0x1C]=currFile.fileSize & 0xFF;
				buffer[offset+0x1D]=(currFile.fileSize & 0xFF00)>>8;
				buffer[offset+0x1E]=(currFile.fileSize & 0xFF0000)>>16;
				buffer[offset+0x1F]=currFile.fileSize>>24;

				mmc::writeSector(buffer, currSec);

				done=true;
			}
		}

		return NO_ERROR;
	}
	else
		if (currFile.fileMode==0x00)
			return ERROR_NO_FILE_OPEN;
		else
			return ERROR_WRONG_FILEMODE;
}

void tinyFAT::closeFile()
{
	currFile.filename[0]=0x00;
	currFile.fileMode=0x00;
}

boolean tinyFAT::exists(char *fn)
{
	_directory_entry tmpDE;
	char tmpFN[13];
	byte res;
	int i, j;

	for (i=0; i<strlen(fn); i++)
		fn[i]=uCase(fn[i]);

	res=findFirstFile(&tmpDE);
	if (res==ERROR_NO_MORE_FILES)
		return false;
	else
	{
		i=0;
		j=0;
		while ((tmpDE.filename[i]!=0x20) and (i<8))
		{
			tmpFN[i]=tmpDE.filename[i];
			i++;
		}
		tmpFN[i]='.';
		i++;
		while ((tmpDE.fileext[j]!=0x20) and (j<3))
		{
			tmpFN[i]=tmpDE.fileext[j];
			i++;
			j++;
		}
		tmpFN[i]=0x00;
		if (!strcmp(tmpFN,fn))
			return true;
		while (res==NO_ERROR)
		{
			res = file.findNextFile(&tmpDE);
			if (res==NO_ERROR)
			{
				i=0;
				j=0;
				while ((tmpDE.filename[i]!=0x20) and (i<8))
				{
					tmpFN[i]=tmpDE.filename[i];
					i++;
				}
				tmpFN[i]='.';
				i++;
				while ((tmpDE.fileext[j]!=0x20) and (j<3))
				{
					tmpFN[i]=tmpDE.fileext[j];
					i++;
					j++;
				}
				tmpFN[i]=0x00;
				if (!strcmp(tmpFN,fn))
					return true;
			}
		}
	}
	return false;
}

boolean tinyFAT::rename(char *fn1, char *fn2)
{
	unsigned long currSec = firstDirSector;
	word offset = -32;
	char tmpFN[13];
	int i, j;
	boolean done=false;

	for (i=0; i<strlen(fn1); i++)
		fn1[i]=uCase(fn1[i]);

	for (i=0; i<strlen(fn2); i++)
	{
		fn2[i]=uCase(fn2[i]);
		if (!validChar(fn2[i]))
			return false;
	}

	if (exists(fn1))
	{
		mmc::readSector(buffer, currSec);
		while (!done)
		{
			offset+=32;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 

			j=0;
			for (int i=0; i<8; i++)
			{
				if (buffer[i+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+offset];
					j++;
				}
			}
			tmpFN[j]='.';
			j++;
			for (int i=0; i<3; i++)
			{
				if (buffer[i+0x08+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+0x08+offset];
					j++;
				}
			}
			tmpFN[j]=0x00;
			if (!strcmp(tmpFN, fn1))
			{
				for (int i=0; i<11; i++)
				{
					buffer[i+offset]=0x20;
				}
				j=0;
				for (int i=0; i<strlen(fn2); i++)
				{
					if (fn2[i]=='.')
						j=8;
					else
					{
						buffer[j+offset]=fn2[i];
						j++;
					}
				}
				mmc::writeSector(buffer, currSec);
				done=true;
			}
		}

		return true;
	}
	else
		return false;
}

boolean tinyFAT::delFile(char *fn)
{
	unsigned long currSec = firstDirSector;
	uint16_t firstCluster, currCluster, nextCluster;
	word offset = -32;
	char tmpFN[13];
	int j;
	boolean done=false;

	for (int i=0; i<strlen(fn); i++)
		fn[i]=uCase(fn[i]);

	if (exists(fn))
	{
		mmc::readSector(buffer, currSec);
		while (!done)
		{
			offset+=32;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 

			j=0;
			for (int i=0; i<8; i++)
			{
				if (buffer[i+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+offset];
					j++;
				}
			}
			tmpFN[j]='.';
			j++;
			for (int i=0; i<3; i++)
			{
				if (buffer[i+0x08+offset]!=0x20)
				{
					tmpFN[j]=buffer[i+0x08+offset];
					j++;
				}
			}
			tmpFN[j]=0x00;
			if (!strcmp(tmpFN, fn))
			{
				buffer[offset]=0xE5;
				firstCluster = uint16_t(buffer[0x1A + offset]) + (uint16_t(buffer[0x1B + offset])<<8);
				mmc::writeSector(buffer, currSec);
				
				if (firstCluster!=0)
				{
					currSec=firstCluster/256;
					mmc::readSector(buffer, BS.fat1Start+currSec);
					currCluster=firstCluster;
					nextCluster=0;
					while (nextCluster!=0xFFFF)
					{
						nextCluster = buffer[(currCluster % 256)*2] + (buffer[((currCluster % 256)*2)+1]<<8);
						buffer[(currCluster % 256)*2]=0;
						buffer[((currCluster % 256)*2)+1]=0;
						if (((currCluster/256) != (nextCluster/256)) and (nextCluster!=0xFFFF))
						{
							mmc::writeSector(buffer, BS.fat1Start+currSec);
							currSec=nextCluster/256;
							mmc::readSector(buffer, BS.fat1Start+currSec);

						}
						currCluster=nextCluster;
					}
					mmc::writeSector(buffer, BS.fat1Start+currSec);

					currSec=firstCluster/256;
					mmc::readSector(buffer, BS.fat2Start+currSec);
					currCluster=firstCluster;
					nextCluster=0;
					while (nextCluster!=0xFFFF)
					{
						nextCluster = buffer[(currCluster % 256)*2] + (buffer[((currCluster % 256)*2)+1]<<8);
						buffer[(currCluster % 256)*2]=0;
						buffer[((currCluster % 256)*2)+1]=0;
						if (((currCluster/256) != (nextCluster/256)) and (nextCluster!=0xFFFF))
						{
							mmc::writeSector(buffer, BS.fat2Start+currSec);
							currSec=nextCluster/256;
							mmc::readSector(buffer, BS.fat2Start+currSec);

						}
						currCluster=nextCluster;
					}
				mmc::writeSector(buffer, BS.fat2Start+currSec);
				}

				done=true;
			}
		}

		return true;
	}
	else
		return false;
}

boolean tinyFAT::create(char *fn)
{
	unsigned long currSec;
	word offset = 0;
	boolean done=false;
	int j;

	for (int i=0; i<strlen(fn); i++)
	{
		fn[i]=uCase(fn[i]);
		if (!validChar(fn[i]))
			return false;
	}

	if (!exists(fn))
	{
		currSec = firstDirSector;
		mmc::readSector(buffer, currSec);
		offset = -32;
		while (!done)
		{
			offset+=32;
			if (offset==512)
			{
			  currSec++;
			  mmc::readSector(buffer, currSec);
			  offset = 0;
			} 

			if ((buffer[offset]==0x00) or (buffer[offset]==0xE5))
			{
				for (int i=0; i<11; i++)
				{
					buffer[i+offset]=0x20;
				}
				j=0;
				for (int i=0; i<strlen(fn); i++)
				{
					if (fn[i]=='.')
						j=8;
					else
					{
						buffer[j+offset]=fn[i];
						j++;
					}
				}
				
				for (int i=0x0b; i<0x20; i++)
					buffer[offset+i]=0;
				buffer[offset+0x0b]=0x20;
				buffer[offset+0x0f]=0x60;
				buffer[offset+0x10]=0x21;
				buffer[offset+0x11]=0x3E;
				buffer[offset+0x12]=0x21;
				buffer[offset+0x13]=0x3E;
				buffer[offset+0x17]=0x60;
				buffer[offset+0x18]=0x21;
				buffer[offset+0x19]=0x3E;

				mmc::writeSector(buffer, currSec);

				done=true;
			}
		}
		return true;
	}
	else
		return false;
}

/* Private */

uint16_t tinyFAT::findNextCluster(uint16_t cc)
{
	uint16_t nc;
	mmc::readSector(buffer, BS.fat1Start+int(cc/256));
	nc = buffer[(cc % 256)*2] + (buffer[((cc % 256)*2)+1]<<8);
	return nc;
}

char tinyFAT::uCase(char c)
{
	if ((c>='a') && (c<='z'))
		return (c-0x20);
	else
		return c;
}

boolean tinyFAT::validChar(char c)
{
	char valid[]= "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$&'()-@^_`{}~.";

	for (int i=0; i<strlen(valid); i++)
		if (c==valid[i])
			return true;
	return false;
}

uint16_t tinyFAT::findFreeCluster()
{
	unsigned long currSec=0;
	word firstFreeCluster=0;
	word offset=0;

	while ((firstFreeCluster==0) and (currSec<=BS.sectorsPerFAT))
	{
		mmc::readSector(buffer, BS.fat1Start+currSec);
		while ((firstFreeCluster==0) and (offset<=512))
		{
			if ((buffer[offset] + (buffer[offset+1]<<8))==0)
				firstFreeCluster=(currSec<<8)+(offset/2);
			else
				offset+=2;
		}
		offset=0;
		currSec++;
	}
}

void tinyFAT::setSSpin(byte pin)
{
	if (_inited==false)
		mmc::setSSpin(pin);
}

/* Declarations */

tinyFAT file = tinyFAT();