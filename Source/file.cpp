#include <string>
#include <unordered_map>

extern "C" {
#include "file.h"
}

namespace {
    struct File {
        uint8_t* ptr = nullptr;
        size_t size = 0;
    };
}

static std::unordered_map<std::string, File> files;

static int COM_FOpenFile (const char *filename, FILE **file)
{
	searchpath_t	*search;
	pack_t		*pak;
	int		i;

	com_filesize = -1;
	com_netpath[0] = 0;

	// search through the path, one element at a time
	for (search = com_searchpaths ; search ; search = search->next)
	{
		// is the element a pak file?
		if (search->pack)
		{
			// look through all the pak file elements
			pak = search->pack;
			for (i=0 ; i<pak->numfiles ; i++)
			{
				if (!strcmp(pak->files[i].name, filename))	// found it!
				{
					if (developer.value)
						Sys_Printf ("PackFile: %s : %s\n", pak->filename, filename);
					// open a new file on the pakfile
					if (!(*file = fopen(pak->filename, "rb")))
						Sys_Error ("Couldn't reopen %s", pak->filename);
					fseek (*file, pak->files[i].filepos, SEEK_SET);
					com_filesize = pak->files[i].filelen;

					Q_snprintfz (com_netpath, sizeof(com_netpath), "%s#%i", pak->filename, i);
					return com_filesize;
				}
			}
		}
		else
		{               
			// check a file in the directory tree
			Q_snprintfz (com_netpath, sizeof(com_netpath), "%s/%s", search->filename, filename);

			if (!(*file = fopen(com_netpath, "rb")))
				continue;

			if (developer.value)
				Sys_Printf ("FOpenFile: %s\n", com_netpath);

			com_filesize = COM_FileLength (*file);
			return com_filesize;
		}
		
	}

	if (developer.value)
		Sys_Printf ("COM_FOpenFile: can't find %s\n", filename);

	*file = NULL;
	com_filesize = -1;

	return -1;
}


static int LoadFile(const std::string& ident, const char* filename, const char* mode) {
    FILE* handle;
    int size = COM_FOpenFile(filename, &handle);
    if(size == -1) {
        return -1;
    }
    File output;
    output.ptr = (uint8_t*)malloc(size);
    uint8_t BUFFER[4096];
    size_t bytesRead = 0;

    while(output.size < size) {
        size_t bytesLeft = min(sizeof(BUFFER), size - output.size);
        bytesRead = fread(BUFFER, 1, bytesLeft, handle);
        if(bytesRead == 0) {
            break;
        } else {
            memcpy(output.ptr + output.size, BUFFER, bytesRead);
            output.size += bytesRead;
        }
    }

    files[ident] = output;
    fclose(handle);

    return 0;
}

Q_File* Q_fopen(const char* filename, const char* mode) {
    std::string ident = filename;
    ident += mode;

    if(files.find(ident) == files.end()) {
        if(LoadFile(ident, filename, mode) == -1) {
            return NULL;
        }
    }

    Q_File* output = (Q_File*)malloc(sizeof(Q_File));
    output->ptr = files[ident].ptr;
    output->offset = 0;
    output->size = files[ident].size;
    return output;
}

void Q_fclose(Q_File* file) {
    free(file);
}