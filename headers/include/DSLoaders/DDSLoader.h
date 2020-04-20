#ifndef DDSLOADER_H
#define DDSLOADER_H

#include "Loader.h"
#include <string>

class DDSLoader : public Loader
{
public:
    uint8_t *loadFile(const std::string &filename) override;
	bool writeRawFile(const std::string &filename, uint8_t *pVoldata, uint32_t VoldataSize);
};

#endif // DDSLOADER_H
