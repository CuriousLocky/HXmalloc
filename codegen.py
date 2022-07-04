from jinja2 import Template
import os

# check generate dir existence
genSrcPath = "generated/src/"
genIncludePath = "generated/include/"

if(not os.path.exists(genSrcPath)):
    os.makedirs(genSrcPath)
if(not os.path.exists(genIncludePath)):
    os.makedirs(genIncludePath)
    
# generate small block rules
smallBlockSizes = [16, 32, 48, 64]

for blockSize in smallBlockSizes:
    with open("src/smallBlockTemplate.c") as templateFile:
        template = Template(templateFile.read())
        srcFileName = genSrcPath + "smallBlock{blockSize}.c".format(blockSize = blockSize)
        with open(srcFileName, 'w') as outputFile:
            outputFile.write(template.render(blockSize = blockSize))
    with open("include/smallBlockTemplate.h") as templateFile:
        template = Template(templateFile.read())
        includeFileName = genIncludePath + "smallBlock{blockSize}.h".format(blockSize = blockSize)
        with open(includeFileName, "w") as outputFile:
            outputFile.write(template.render(blockSize = blockSize))
    
    
# generate small block header
smallBlockHeaderFileName = genIncludePath + "smallBlock.h"
smallBlockHeader = ""
for blockSize in smallBlockSizes:
    smallBlockHeader += "#include \"smallBlock{blockSize}.h\"\n".format(blockSize = blockSize)
smallBlockHeader += """
void initSmallBlock();
BlockHeader *findSmallVictim(uint64_t size);
void freeSmallBlock(BlockHeader *block, BlockHeader header);
"""    
with open(smallBlockHeaderFileName, "w") as outputFile:
    outputFile.write(smallBlockHeader)


# generate small block source file
smallBlockSrcFileName = genSrcPath + "smallBlock.c"
smallBlockSrc = "#include \"smallBlock.h\""
smallBlockSrc += """
void initSmallBlock(){
{{initBlock}}
}

BlockHeader *findSmallVictim(uint64_t size){
    size = size / 16;
    BlockHeader *(*findVictimFunctions[{{blockSizeNumber}}])() = {
{{findVictimFunctions}}
    };
    return findVictimFunctions[size];
}

void freeSmallBlock(BlockHeader *block, BlockHeader header){
    size_t size = getSize(header)/16;
    void (*freeBlockFunctions[{{blockSizeNumber}}])(BlockHeader *, BlockHeader) = {
{{freeBlockFunctions}}
    };
    (*freeBlockFunctions[size])(block, header);
}
"""       
smallBlockInitFunctions = []
findVictimFunctions = []
freeBlockFunctions = []
for blockSize in smallBlockSizes:
    smallBlockInitFunctions.append("    initBlock{blockSize}();".format(blockSize = blockSize))
    findVictimFunctions.append("        findVictim{blockSize}".format(blockSize = blockSize))
    freeBlockFunctions.append("        freeBlock{blockSize}".format(blockSize = blockSize))
with open(smallBlockSrcFileName, "w") as outputFile:
    template = Template(smallBlockSrc)
    outputFile.write(template.render(
        initBlock='\n'.join(smallBlockInitFunctions), 
        blockSizeNumber=len(smallBlockSizes), 
        findVictimFunctions = ",\n".join(findVictimFunctions),
        freeBlockFunctions = ",\n".join(freeBlockFunctions)
    ))