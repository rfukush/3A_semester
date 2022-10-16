import subprocess
subprocess.call(['wget', 'https://www.cs.toronto.edu/~kriz/cifar-10-python.tar.gz'])
subprocess.call(['tar', 'xvzf', 'cifar-10-python.tar.gz'])
subprocess.call(['rm', 'cifar-10-python.tar.gz'])
