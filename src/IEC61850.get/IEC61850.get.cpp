// IEC61850.get.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "iec61850_client.h"
#include <stdlib.h>
#include <stdio.h>
#include "hal_thread.h"


static bool downloadHandler(void* parameter, uint8_t *buffer, uint32_t bytesRead)
{
	printf("%i, ", bytesRead);

	FILE *f = (FILE*)parameter;

	fwrite(buffer, 1, bytesRead, f);

	return true;   
}


void Download(IedConnection c, _TCHAR* srcpath, _TCHAR* outdir)
{
	_TCHAR *dstfile = _tcsrchr(srcpath, '/');

	if (dstfile == NULL)
		dstfile = srcpath;
	else
		dstfile++;

	_TCHAR dstpath[0x1000];
	dstpath[0] = 0;
	_tcscat(dstpath, outdir);
	_tcscat(dstpath, dstfile);

	IedClientError error;

	printf("Downloading %s, ", srcpath, dstpath);
		
	FILE *f = fopen(dstpath, "w+");

	IedConnection_getFile(c, &error, srcpath, downloadHandler, f);

	if (error != IED_ERROR_OK)
		printf("Failed to get file! Error=%d\n", error);
	else		
		printf("Done\n");

	fclose(f);
}



void Delete(IedConnection c, _TCHAR *path)
{
	IedClientError error;

	/* Delete file at server */
	IedConnection_deleteFile(c, &error, path);

	if (error != IED_ERROR_OK)
		printf("Failed to delete '%s' file! (code=%i)\n", path, error);

}


//usage: IEC61850.get 

int _tmain(int argc, _TCHAR* argv[])
{	
	int tcpPort = 102;
	bool usage = argc < 4;
	_TCHAR *hostname;
	_TCHAR *path;
	_TCHAR *outdir;
	bool moveFile; 
	bool dir;
	bool get;
	bool mfiles;


	if (!usage)
	{
		hostname = argv[1];
		moveFile = !_tccmp(argv[2], _T("move"));		
		get = !_tccmp(argv[2], _T("get")) || moveFile;
		dir = !_tccmp(argv[2], _T("dir"));

		path = argv[3];

		int pathlen = _tcslen(path);
		mfiles = (path[pathlen-1] == '/');

		if (mfiles)
		{
			path[pathlen-1] = 0;
		}

		if (argc == 5) {
			
			int len = _tcslen(argv[4]);
			if (len != 0 && argv[4][len -1] != '\\')
			{
				outdir = new char[len + 2];
				_tcscat(outdir, argv[4]);
				_tcscat(outdir, _T("\\"));
			} else
				outdir = argv[4];


		} else
			outdir = ".\\";
	}

	if (usage)
	{
		printf("usage: IEC61850.get hostname <get|dir|move> remotepath\n");
		return 1;	
	}

	IedClientError error;

	IedConnection con = IedConnection_create();

	IedConnection_connect(con, &error, hostname, tcpPort);

	if (error != IED_ERROR_OK) {
		fprintf(stderr, "Connection refused: %s:%d error=%d\n", hostname, tcpPort, error);
		return 1;
	}


	if (dir || mfiles)
	{
		
		LinkedList rootDirectory = IedConnection_getFileDirectory(con, &error, path);

		if (error != IED_ERROR_OK) {
			fprintf(stderr, "Error retrieving file directory\n");
		} else {

			LinkedList directoryEntry = LinkedList_getNext(rootDirectory);

			while (directoryEntry != NULL) {

				FileDirectoryEntry entry = (FileDirectoryEntry) directoryEntry->data;
				_TCHAR *filename = FileDirectoryEntry_getFileName(entry);

				if (get)
				{
					Download(con, filename, outdir); 
				} 
				if (dir) {
					printf("%s\t%i\n", filename, FileDirectoryEntry_getFileSize(entry));
				}
				if (moveFile)
				{
					Delete(con, filename);
				}

				directoryEntry = LinkedList_getNext(directoryEntry);
			}

		}
	} else 
	{

		if (get)
		{
			Download(con, path, outdir); 
		} 

		if (moveFile)
		{
			Delete(con, path);
		}
	}

	IedConnection_destroy(con);
}

