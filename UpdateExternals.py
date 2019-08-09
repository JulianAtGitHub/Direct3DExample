import os
import sys
import shutil
from subprocess import check_output

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
EXTERNAL_DIR = CURRENT_DIR + os.path.sep + "Externals"
REVISION_FILE = EXTERNAL_DIR + os.path.sep + "Revisions"

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
        check_output(["git", "clone", self.url, "."])
        check_output(["git", "checkout", self.revision])

    def Update(self):
        os.chdir(self.path)
        check_output(["git", "fetch", "--all"])
        check_output(["git", "checkout", "--force", self.revision])


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


