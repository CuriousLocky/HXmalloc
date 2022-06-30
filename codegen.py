from jinja2 import Template
import os

blockSize = 16

genSrcPath = "generated/src/"
genIncludePath = "generated/include/"

if(not os.path.exists(genSrcPath)):
    os.makedirs(genSrcPath)

with open("src/smallBlockTemplate.c") as templateFile:
    template = Template(templateFile.read())
    srcFileName = genSrcPath + "smallBlock{blockSize}.c".format(blockSize = blockSize)
    with open(srcFileName, 'w') as outputFile:
        outputFile.write(template.render(blockSize = blockSize))