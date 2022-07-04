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
smallBlockHeaderFileName = genIncludePath + "smallBlock.h"
smallBlockHeader = ""
smallBlockSizes = [16, 32, 64]

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
    smallBlockHeader += "#include \"smallBlock{blockSize}.h\"\n".format(blockSize = blockSize)
with open(smallBlockHeaderFileName, "w") as outputFile:
    outputFile.write(smallBlockHeader)