import chunk
from jinja2 import Template
import os

# check generate dir existence
genSrcPath = "generated/src/"
genIncludePath = "generated/include/"

if(not os.path.exists(genSrcPath)):
    os.makedirs(genSrcPath)
if(not os.path.exists(genIncludePath)):
    os.makedirs(genIncludePath)
    
# generate small block specific code
smallBlockSizes = [16, 32, 48, 64]
smallBlockSrcSpecificTemplate = ""
smallBlockHeaderSpecificTemplate = ""
with open("templates/smallBlockSpecificTemplate.c") as templateFile:
    smallBlockSrcSpecificTemplate = Template(templateFile.read())
with open("templates/smallBlockSpecificTemplate.h") as templateFile:
    smallBlockHeaderSpecificTemplate = Template(templateFile.read());
for blockSize in smallBlockSizes:
    srcFileName = genSrcPath + "smallBlock{blockSize}.c".format(blockSize = blockSize)
    chunkSize = 4096 + (510*64*blockSize)&(~4095)
    with open(srcFileName, 'w') as outputFile:
        outputFile.write(smallBlockSrcSpecificTemplate.render(blockSize = blockSize, chunkSize = chunkSize))
    includeFileName = genIncludePath + "smallBlock{blockSize}.h".format(blockSize = blockSize)
    with open(includeFileName, "w") as outputFile:
        outputFile.write(smallBlockHeaderSpecificTemplate.render(blockSize = blockSize))
    
# # generate small block header
smallBlockHeaderTemplate = ""
with open("templates/smallBlockTemplate.h") as templateFile:
    smallBlockHeaderTemplate = Template(templateFile.read())
smallBlockHeaderFileName = genIncludePath + "smallBlock.h"
smallBlockMetaData = []
for blockSize in smallBlockSizes:
    smallBlockMetaData += [
        "    NonBlockingStackBlock cleanSuperBlock{blockSize}Stack;".format(blockSize = blockSize),
        "    uint64_t *localSuperBlock{blockSize};".format(blockSize = blockSize),
        "    uint64_t *localSuperBlock{blockSize}BitMap;".format(blockSize = blockSize),
        "    uint64_t *chunk{blockSize};".format(blockSize = blockSize),
        "    uint64_t chunk{blockSize}Usage;".format(blockSize = blockSize),
        "    unsigned int managerPageUsage{blockSize};".format(blockSize = blockSize)
    ]
with open(smallBlockHeaderFileName, "w") as outputFile:
    outputFile.write(smallBlockHeaderTemplate.render(
        smallBlockMetaData = "\n".join(smallBlockMetaData)
    ))


# generate small block source file
smallBlockSrcTemplateFileName = "templates/smallBlockTemplate.c"
smallBlockSrcFileName = genSrcPath + "smallBlock.c"
smallBlockSrcTemplate = ""
with open(smallBlockSrcTemplateFileName) as templateFile:
    smallBlockSrcTemplate = Template(templateFile.read())
smallBlockInitFunctions = []
findVictimFunctions = []
freeBlockFunctions = []
smallBlockHeaders = []
for blockSize in smallBlockSizes:
    smallBlockHeaders.append("#include \"smallBlock{blockSize}.h\"".format(blockSize = blockSize))
    smallBlockInitFunctions.append("    initBlock{blockSize}();".format(blockSize = blockSize))
    findVictimFunctions.append("        findVictim{blockSize}".format(blockSize = blockSize))
    freeBlockFunctions.append("        freeBlock{blockSize}".format(blockSize = blockSize))
with open(smallBlockSrcFileName, "w") as outputFile:
    outputFile.write(smallBlockSrcTemplate.render(
        initBlock="\n".join(smallBlockInitFunctions), 
        blockSizeNumber=len(smallBlockSizes), 
        findVictimFunctions = ",\n".join(findVictimFunctions),
        freeBlockFunctions = ",\n".join(freeBlockFunctions),
        smallBlockHeaders = "\n".join(smallBlockHeaders)
    ))