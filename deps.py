import os
import subprocess
from pathlib import Path

def main():
    gitit('Box2D', 'https://github.com/erincatto/box2d.git')

def gitit(pkg, url):
    path = os.getcwd()
    os.chdir("./deps")
    if not Path('.').glob(pkg+'*'):
        print(pkg + ' is')
        subprocess.run(["git", "clone", url], shell=True)
    else:
        print(pkg + ' not')   
    os.chdir(path)

if __name__ == "__main__":
    main()

