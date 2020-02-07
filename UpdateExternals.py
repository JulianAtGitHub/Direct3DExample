import os
import sys
import shutil
from subprocess import check_output

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
EXTERNAL_DIR = CURRENT_DIR + os.path.sep + "Externals"
REVISION_FILE = EXTERNAL_DIR + os.path.sep + "Revisions"
BUILD_FOLDER = "build"
BUILD_ARCH = "Visual Studio 15 2017 Win64"

class Repository:
    def __init__(self, path, name, url, revision):
        self.path = path + os.path.sep + name
        self.name = name
        self.url = url
        self.revision = revision

    def Exist(self):
        if os.path.exists(self.path) == True and os.path.exists(self.path + os.path.sep + ".git") == True:
            return True
        return False

    def Create(self):
        # remove folder if exist
        if os.path.exists(self.path):
            shutil.rmtree(self.path)
        os.makedirs(self.path)
        os.chdir(self.path)
        os.system("git clone " + self.url + " .")
        os.system("git checkout " + self.revision)

    def Update(self):
        os.chdir(self.path)
        os.system("git fetch --all")
        os.system("git checkout --force " + self.revision)

with open(REVISION_FILE, "rb") as file:
   for line in file:
        params = line.split(" ")
        if len(params) >= 3:
            repo = Repository(EXTERNAL_DIR, params[0], params[1], params[2])
            if repo.Exist() == True:
                repo.Update()
            else:
                repo.Create()
            os.chdir(CURRENT_DIR)

# build assimp
os.chdir(EXTERNAL_DIR + os.path.sep + "assimp")
if os.path.exists(BUILD_FOLDER) == False:
    os.mkdir(BUILD_FOLDER)
os.chdir(BUILD_FOLDER)
print check_output(["cmake", "..", "-G", "Visual Studio 15 2017 Win64", "-DCMAKE_INSTALL_PREFIX=Debug"])
os.system("cmake --build . --config Debug --target install")
print check_output(["cmake", "..", "-G", "Visual Studio 15 2017 Win64", "-DCMAKE_INSTALL_PREFIX=Release"])
os.system("cmake --build . --config Release --target install")

