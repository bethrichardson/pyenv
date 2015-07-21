The bootstrap script for PyEnv can be found at 
https://raw.githubusercontent.com/cschcs/pyenv-bootstrap/master/get-pyenv.sh 
which is located in the public https://github.com/cschcs/pyenv-bootstrap repository.

Alternatively a short URL can be used: http://tinyurl.com/getpyenv

**get-pyenv.sh** script behaves as follows:

1. If there are command-line arguments passed to the script and the first argument is 
non-empty, the script treats the first argument as the name of a branch or a tag 
containing the desired environment.

2. If there are no command-line arguments, if there is a *PYENV_VERSION* environmental 
variable set and non-empty, the script treats *PYENV_VERSION* variable as the name of 
a branch or a tag containing the desired environment.

3. If there are no command-line argument and there is no *PYENV_VERSION* environmental 
variable, the script checks for *.pyenv* file in the current directory. If the file 
exists, its contents are treated as the name of a branch or a tag containing the 
desired environment.

4. In all other cases the script defaults to *"master"* as the name of a branch containing 
the desired environment.

Upon making a decision on what version of environment to pull, **get-pyenv.sh** (re-)creates 
a directory *$HOME/pyenv/&lt;branch/tag&gt;* and pulls the copy of the PyEnv environment to that 
directory.

If successful, a path *$HOME/pyenv/&lt;branch/tag&gt;* is printed to stdout and script exits 
with code 0. In all other cases the output goes to stderr and non-zero exit code is set.

**get-pyenv.sh** also drops itself into *$HOME/pyenv/get-pyenv.sh* and if invoked there 
will self-update.

Here are a few cross-platform shortcuts for bootstrapping PyEnv environment:
 
Command	| Description
------- | -----------
bash <(curl -L -s 'http://tinyurl.com/getpyenv') | Download bootstrap script and execute it in the current directory printing the local of the PyEnv DevEnv scripts to stdout
"$(bash <(curl -L -s 'http://tinyurl.com/getpyenv'))/setup.sh" | Download bootstrap script, execute it in the current directory and run full environment setup

