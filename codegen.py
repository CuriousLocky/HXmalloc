from jinja2 import Template
import os

# generate small block rules
blockSize = 16

genSrcPath = "generated/src/"
genIncludePath = "generated/include/"

if(not os.path.exists(genSrcPath)):
    os.makedirs(genSrcPath)
if(not os.path.exists(genIncludePath)):
    os.makedirs(genIncludePath)

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