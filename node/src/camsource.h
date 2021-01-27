#pragma once

#include <windows.h>
struct camsource {
	HANDLE hMapFile, hMapFile2, m_hMutex;
	BYTE* pBuf;
	BITMAPINFOHEADER* bi;

	unsigned size;

char memnamedata[1024];
char memnameconfig[1024];
char memnamelock[1024];

	bool fastget;

	camsource() :hMapFile(NULL), hMapFile2(NULL), m_hMutex(NULL), pBuf(NULL), bi(NULL), size(0), fastget(false)
	{

	}

	~camsource() {
		close();
	}

	void close() {
		if (pBuf) UnmapViewOfFile(pBuf);
		if (hMapFile) CloseHandle(hMapFile);
		if (bi) UnmapViewOfFile(bi);
		if (hMapFile2) CloseHandle(hMapFile2);
		if (m_hMutex) CloseHandle(m_hMutex);
		pBuf = NULL;
		hMapFile = NULL;
		bi = NULL;
		hMapFile2 = NULL;
		m_hMutex = NULL;
	}

	bool init(BITMAPINFOHEADER* _bi,const char *_memnamedata,const char *_memnameconfig,const char *_memnamelock) {

		strcpy(memnamedata,_memnamedata);
		strcpy(memnameconfig,_memnameconfig);
		strcpy(memnamelock,_memnamelock);


		hMapFile2 = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memnameconfig);
		if (hMapFile2 != NULL) {			
			bi = (BITMAPINFOHEADER*)MapViewOfFile(hMapFile2, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BITMAPINFOHEADER));
			m_hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, memnamelock);
			memcpy(_bi, bi, sizeof(BITMAPINFOHEADER));
			return true;
		}
		return false;
	}

	void* get(BITMAPINFOHEADER* _bi) 
	{
		void* ret = NULL;
		if (bi && m_hMutex) {
			int newsize = bi->biWidth * bi->biHeight * bi->biBitCount / 8;
			if (newsize != size) {				
				if (pBuf) UnmapViewOfFile(pBuf);
				if (hMapFile != INVALID_HANDLE_VALUE) CloseHandle(hMapFile);
				hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, memnamedata);
				if (hMapFile != NULL)
				{
					pBuf = (BYTE*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, newsize);
					size = newsize;
				}
			}

			if (bi) {
				memcpy(_bi, bi, sizeof(BITMAPINFOHEADER));
			}

			if (pBuf) {
				if (!fastget) {
					WaitForSingleObject(m_hMutex, INFINITE);
				}
				ret = pBuf;
			}
		}
		return ret;
	}

	void release()
	{
		if (!fastget) {
			ReleaseMutex(m_hMutex);
		}
	}
};