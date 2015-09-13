#!/bin/bash -eEu

readonly OSNAME=$(uname -s)
readonly ARCH=$(uname -m)

case "$OSNAME" in
    Linux)
        BIN_WHICH="/dev/null"
        if [ -e "/bin/which" ]; then
          BIN_WHICH="/bin/which"
        elif [ -e "/usr/bin/which" ]; then
          BIN_WHICH="/usr/bin/which"
        fi
        which()
        {
          "$BIN_WHICH" --skip-alias --skip-functions "$@"
        }
        readlink()
        {
          /bin/readlink -nf "$@"
        }
        perlgrep()
        {
          /bin/grep -P -o "$@"
        }
        sha256sum()
        {
          /usr/bin/sha256sum "$@"
        }
        sha1sum()
        {
          /usr/bin/sha1sum "$@"
        }
        sed_inplace()
        {
          /bin/sed -i -e "$@"
        }
        sed_inplace_r()
        {
          /bin/sed -i -r "$@"
        }
        sed_inplace_sudo()
        {
          sudo /bin/sed -i -e "$@"
        }
        sed_inplace_r_sudo()
        {
          sudo /bin/sed -i -r "$@"
        }
        sed_extended()
        {
          /bin/sed -r "$@"
        }
        echo "Detected Linux..."
        ;;
    Darwin)
      echo "Detected Mac OS X (Darwin)..."
      which()
      {
        set -eEu
        /usr/bin/which "$@"
      }
      readlink()
      {
        set -eEu
        local P="$1"
        P_DIR=""
        P_FILE=""
        if [ -e "$P" ]; then
        if [ -h "$P" ]; then
            P="$(dirname "$P")/$(/usr/bin/readlink "$P")"
        fi
        if [ -d "$P" ]; then
            P="$(cd "$P" >/dev/null 2>&1 && pwd -P)"
            echo -n "$P"
            return 0
        fi
        fi
        P_DIR="$(dirname "$P" 2>/dev/null)"
        P_FILE="${P#$P_DIR/}"
        P_DIR="$(cd "$P_DIR" >/dev/null 2>&1 && pwd -P)"
        echo -n "$P_DIR/$P_FILE"
      }
      perlgrep()
      {
        set -eEu
        perl -le'$found=1; while(<>) { while( $string = m{'"$@"'}g ) { print $&; $found=0; } } exit $found;'
      }
      sha256sum()
      {
        set -eEu
        /usr/bin/shasum -a256 "$@"
      }
      sha1sum()
      {
        /usr/bin/shasum -a1 "$@"
      }
      base64()
      {
        set -eEu
        /usr/bin/base64
      }
      sed_inplace()
      {
        set -eEu
        /usr/bin/sed -i "" "$@"
      }
      sed_inplace_r()
      {
        sed_inplace "$@"
      }
      sed_inplace_sudo()
      {
        set -eEu
        sudo /usr/bin/sed -i "" "$@"
      }
      sed_inplace_r_sudo()
      {
        sed_inplace_sudo "$@"
      }
      sed_extended()
      {
        set -eEu
        /usr/bin/sed -E "$@"
      }
      ;;
    *)
      echo "Linux or Mac OS X only for now! $OSNAME is not yet supported."
      exit 1
      ;;
esac

readonly FILE_TS="date +%FT%H-%M-%S"
readonly PYTHON_USER_BASE=$(python <<EOF
import site
print(site.USER_BASE)
EOF
)
readonly PYHOME="$HOME/pyhome"

#
# $1 - file name
# $2 - use sudo (optional, set to any non-zero value)
#
function backup-file()
{
    set -eEu

    if [ ! -e "$1" ]; then
        return 0
    fi

    local FILE="$(readlink "$1")"
    local FILE_NAME="${FILE##*/}"

    local SUDO=""
    if [ -n "${2:-}" ]; then
        SUDO="sudo"
    fi

    BAK_FILE="$(dirname "$FILE")/$FILE_NAME.$($FILE_TS)${FILE_EXT:+.$FILE_EXT}.bak"
    $SUDO cp "$FILE" "$BAK_FILE"
    for f in `find "$(dirname "$FILE")" -maxdepth 1 -type f \( -name "$FILE_NAME"'.????-??-??T??-??-??'.bak \
        -o -name "$FILE_NAME"'.????-??-??-??-??-??'.bak \) | sort -r | tail -n +5`; do
        $SUDO rm "$f"
    done
    echo -n "$BAK_FILE"
}

function install-prerequisites()
{
    set -eEu

    case "$OSNAME" in
      Linux)
        sudo yum -y -q install curl patch git ruby
        ;;
      Darwin)
        :
        ;;
    esac
}

function install-virtualenv()
{
    set -eEu

    if [ -n "${VIRTUAL_ENV:-}" ]; then
      echo "Please deactivate VENV '$VIRTUAL_ENV' before (re-)installing!" >&2
      exit 1
    fi
    echo "Installing VirtualEnv" >&2
    VENV_VERSION="$(cat "$SCRIPT_DIR/venv-version")"
    VENV_CACHE="$PYHOME/venv-cache"
    mkdir -p "$VENV_CACHE"
    (
      cd "$VENV_CACHE"
      curl -L -s -z virtualenv-$VENV_VERSION.tar.gz -R -O "https://pypi.python.org/packages/source/v/virtualenv/virtualenv-$VENV_VERSION.tar.gz"
    )
    VENV_INSTALL_DIR="$(mktemp -d -t virtualenv.XXXXXXXXXX)"
    (
      cd "$VENV_INSTALL_DIR"
      tar -xzf "$VENV_CACHE/virtualenv-$VENV_VERSION.tar.gz"
      cd "$VENV_INSTALL_DIR/virtualenv-$VENV_VERSION"
      python setup.py install --user
    )
    RESULT=$?
    rm -rf "$VENV_INSTALL_DIR"
    [ $RESULT -eq 0 ] || return $RESULT
}

function install-pyhome()
{
    set -eEu

    ENV_NAME="${1:-$(cd "$SCRIPT_DIR" && git rev-parse --abbrev-ref HEAD)}"
    rm -rf "$PYHOME/$ENV_NAME" || :
    mkdir -p "$PYHOME/$ENV_NAME"
    mkdir -p "$PYHOME/$ENV_NAME/logs"
    mkdir -p "$PYHOME/$ENV_NAME/db"
    rm -rf "$PYHOME/current" || :
    (cd "$PYHOME" && ln -s "$ENV_NAME" "current") || exit $?
    (cd "$PYHOME/current" && ln -s "$SCRIPT_DIR" "pyenv") || exit $?
    echo "Created PyHome '$ENV_NAME' from '$SCRIPT_DIR'" >&2

    "$PYTHON_USER_BASE"/bin/virtualenv ~/pyhome/current
    cp "$SCRIPT_DIR"/pip.conf ~/pyhome/current
    mkdir -p "$PYHOME/$ENV_NAME/bin/ext.d"
    cp "$SCRIPT_DIR/ext.d/"{de,}activate_*_venv "$PYHOME/$ENV_NAME/bin/ext.d/"
    patch -d "$PYHOME/$ENV_NAME/bin" < "$SCRIPT_DIR/activate.patch"
}

function configure-bash-profile()
{
    set -eEu

    echo "Customizing Bash profile..."
    BASH_PROFILE="$(echo ~)/.bash_profile"

    BASH_PROFILE_BAK="$(backup-file "$BASH_PROFILE")"
    if [ -f "$BASH_PROFILE" ]; then
      sed_inplace_r "/# PyHome Customization/,/# End PyHome Customization/ d" "$BASH_PROFILE"
    fi

    cat >> "$BASH_PROFILE" <<EOF
# PyHome Customization

EOF
    cat >> "$BASH_PROFILE" <<EOF

alias pyenv="source ~/pyhome/current/bin/activate"

# End PyHome Customization
EOF

}

function activate-venv()
{
    set -eEu

    set +eEu
    source ~/pyhome/current/bin/activate
    set -eEu
}

function deactivate-venv()
{
    set -eEu

    set +eEu
    deactivate
    set -eEu
}

function configure-venv()
{
    set -eEu

    pip install --upgrade -r "$SCRIPT_DIR"/venv-requirements.txt
    pip install --upgrade -r "$SCRIPT_DIR"/requirements.txt
}

function configure-nodeenv()
{
    set -eEu

    echo "Installing Node Env" >&2
    cp "$VIRTUAL_ENV/bin/activate" "$VIRTUAL_ENV/bin/activate.bak"

    nodeenv --verbose -p --prebuilt --iojs --node="$(cat "$SCRIPT_DIR/iojs-version")"
    echo npm version $(npm --version)
    echo nodejs version $(nodejs --version)
    npm install -g code lab

    mv "$VIRTUAL_ENV/bin/activate.bak" "$VIRTUAL_ENV/bin/activate"
    cp "$SCRIPT_DIR/ext.d/"{de,}activate_*_node "$VIRTUAL_ENV/bin/ext.d/"
}

function configure-brew()
{
    set -eEu

    case "$OSNAME" in
      Linux)
        sudo yum -y -q groupinstall 'Development Tools'
        sudo yum -y -q install m4 texinfo bzip2-devel curl-devel expat-devel ncurses-devel zlib-devel
        ;;
      Darwin)
        :
        ;;
    esac

    echo "Installing Brew" >&2
    HOMEBREW_PREFIX="$VIRTUAL_ENV" \
    HOMEBREW_CACHE="$HOMEBREW_PREFIX/../.brew-cache" \
    ruby brew.rb --batch
    cp "$SCRIPT_DIR/ext.d/"{de,}activate_*_brew "$VIRTUAL_ENV/bin/ext.d/"
}
