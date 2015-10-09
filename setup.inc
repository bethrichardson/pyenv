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
        [ -n "${_os_detection_banner:-}" ] || { echo "Detected Linux..." ; _os_detection_banner=x; }
        ;;
    Darwin)
      [ -n "${_os_detection_banner:-}" ] || { echo "Detected Mac OS X (Darwin)..." ; _os_detection_banner=x; }
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
PYHOME="$HOME/pyhome"

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

function assert-not-root()
{
    set -eEu

    [ $(id -u) -ne 0 -a $(id -ru) -ne 0 ] || \
    { echo "PyEnv utilities MUST NOT be run as 'root' directly or via 'su' or 'sudo'." >&2 ; \
      echo "The scripts will drop into 'root' if and when necessary." >&2 ; return 1; }
}

function assert-running-in-venv()
{
    set -eEu

    [ -n "${VIRTUAL_ENV:-}" ] || { echo "Must be run in an activated VirtualEnv!" >&2; return 1; }
}

function install-prerequisites()
{
    set -eEu

    case "$OSNAME" in
      Linux)
        sudo yum -y -q install curl patch git
        ;;
      Darwin)
        if [ -z "$(/usr/bin/xcode-select -print-path 2>/dev/null)" ]; then
            echo "Please install Xcode or Command Line Tools first and try again (see popup)!" >&2
            sudo /usr/bin/xcode-select --install
            return 1
        fi
        ;;
    esac
}

function install-virtualenv()
{
    set -eEu

    if [ -n "${VIRTUAL_ENV:-}" ]; then
        echo "Please deactivate VirtualEnv '$VIRTUAL_ENV' before (re-)installing!" >&2
        return 1
    fi
    echo "Installing VirtualEnv" >&2
    VENV_VERSION="$(cat "$SCRIPT_DIR/venv-version")"
    VENV_CACHE="$PYHOME/.venv-cache"
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
    (cd "$PYHOME" && ln -s "$ENV_NAME" "current") || return $?
    (cd "$PYHOME/current" && ln -s "$SCRIPT_DIR" "pyenv") || return $?
    echo "Created PyHome '$ENV_NAME' from '$SCRIPT_DIR'" >&2

    "$PYTHON_USER_BASE"/bin/virtualenv ~/pyhome/current
    cp "$SCRIPT_DIR"/pip.conf ~/pyhome/current
    mkdir -p "$PYHOME/$ENV_NAME/bin/ext.d"
    cp "$SCRIPT_DIR/ext.d/"{de,}activate_*_venv "$PYHOME/$ENV_NAME/bin/ext.d/"
    patch -d "$PYHOME/$ENV_NAME/bin" < "$SCRIPT_DIR/activate.patch"
    mkdir -p ~/pyhome/current/etc
    cp "$SCRIPT_DIR"/hcsos.config ~/pyhome/current/etc
    cp "$SCRIPT_DIR"/demo_data.json ~/pyhome/current/etc
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

function configure-module-requirements()
{
    set -eEu

    assert-running-in-venv

    pip install --upgrade -r "$SCRIPT_DIR"/$1-requirements.txt
}

function configure-nodeenv()
{
    set -eEu

    echo "Installing NodeEnv..." >&2
    assert-running-in-venv

    cp "$VIRTUAL_ENV/bin/activate" "$VIRTUAL_ENV/bin/activate.bak"

    nodeenv --verbose -p --prebuilt --node="$(cat "$SCRIPT_DIR/node-version")"
    echo npm version $(npm --version)
    echo nodejs version $(nodejs --version)
    npm install -g code lab gulp

    mv "$VIRTUAL_ENV/bin/activate.bak" "$VIRTUAL_ENV/bin/activate"
    install-venv-extension node
}

function configure-macports()
{
    set -eEu

    if [ "$OSNAME" == Darwin ]; then # OSX-only
        echo "Installing MacPorts..." >&2
        assert-running-in-venv

        mkdir -p "$PYHOME"/.macports/distfiles
        mkdir -p "$PYHOME"/.macports/software
        mkdir -p "$PYHOME"/.macports/sources

        MACPORTS_VERSION="$(cat "$SCRIPT_DIR/macports-version")"
        (
            cd "$VENV_CACHE"
            curl -L -s -z "MacPorts-$MACPORTS_VERSION.tar.gz" -R -O "http://downloads.sourceforge.net/project/macports/MacPorts/$MACPORTS_VERSION/MacPorts-$MACPORTS_VERSION.tar.gz"
        )
        RESULT=$?
        [ $RESULT -eq 0 ] || return $RESULT

        MACPORTS_INSTALL_DIR="$(mktemp -d -t macports.XXXXXXXXXX)"
        (
            cd "$MACPORTS_INSTALL_DIR"
            tar -xf "$VENV_CACHE/MacPorts-$MACPORTS_VERSION.tar.gz"
            cd "MacPorts-$MACPORTS_VERSION"

            PATH=/bin:/sbin:/usr/bin:/usr/sbin
            export PATH
            MP_PREFIX="$VIRTUAL_ENV"
            (
              ./configure --prefix="$MP_PREFIX" --with-applications-dir="$MP_PREFIX/Applications" \
                  --with-install-user=$(id -un) --with-install-group=$(id -gn) --enable-readline
              make
              make install
            ) > "$VIRTUAL_ENV"/logs/macports-build.log 2>&1
            RESULT=$?
            [ $RESULT -eq 0 ] || { tail "$VIRTUAL_ENV"/logs/macports-build.log ; echo "Please see more logs in "$VIRTUAL_ENV"/logs/macports-build.log" ; }
            return $RESULT
        )
        RESULT=$?
        rm -rf "$MACPORTS_INSTALL_DIR"
        [ $RESULT -eq 0 ] || return $RESULT

        rm -rf "$VIRTUAL_ENV"/var/macports/distfiles
        rm -rf "$VIRTUAL_ENV"/var/macports/sources
        ln -s "$PYHOME/.macports/distfiles" "$VIRTUAL_ENV"/var/macports/distfiles
        ln -s "$PYHOME/.macports/sources" "$VIRTUAL_ENV"/var/macports/sources


        if [ ! -e "$PYHOME"/.macports/software_key.pem ]; then
            echo "macports" > "$PYHOME"/.macports/key_passphrase
            echo "Creating MacPorts local repository PrivKey..."
            openssl genrsa -des3 -out "$PYHOME"/.macports/software_key.pem -passout file:"$PYHOME"/.macports/key_passphrase 2048
            chmod ug-w "$PYHOME"/.macports/software_key.pem
            echo "Creating MacPorts local repository PubKey..."
            openssl rsa -in "$PYHOME"/.macports/software_key.pem -passin file:"$PYHOME"/.macports/key_passphrase \
                -pubout -out "$PYHOME"/.macports/software_pub_key.pem
        fi

        cat >> "$VIRTUAL_ENV"/etc/macports/macports.conf <<EOF
startupitem_type      none
startupitem_install   no
EOF

        cat >> "$VIRTUAL_ENV"/etc/macports/archive_sites.conf <<EOF
name                    macports_archives

name                    PyEnv
urls                    file://$PYHOME/.macports/software
prefix                  $VIRTUAL_ENV
applications_dir        $VIRTUAL_ENV/Applications
EOF

        cat >> "$VIRTUAL_ENV"/etc/macports/pubkeys.conf <<EOF
$PYHOME/.macports/software_pub_key.pem
EOF

        install-venv-extension macports
        sign_local_macports_repository force
        echo "Syncing MacPorts repository..."
        port -v sync &> "$VIRTUAL_ENV"/logs/macports-port-sync.log
    fi
}

function configure-installer()
{
    set -eEu

    echo "Installing PyEnv Installer..." >&2
    assert-running-in-venv

    install-venv-extension installer
}

function install-binary-packages()
{
    set -eEu

    echo "Installing Binary Packages..." >&2
    assert-running-in-venv

    while read OS COMMANDS; do
        [ -n "$OS" -a "${OS:0:1}" != '#' ] || continue
        install_package "$OS" "$COMMANDS"
    done < "$SCRIPT_DIR/binary-packages"
}

function install-venv-extension() {
    set -eEu

    cp "$SCRIPT_DIR/ext.d/"{de,}activate_*_"$1" "$VIRTUAL_ENV/bin/ext.d/"

    set +eEu
    source "$VIRTUAL_ENV/bin/ext.d/"activate_*_"$1"
    set -eEu
}

function install-frontend() {
    set -eEu

    echo "Installing HCS OS Frontend..." >&2
    assert-running-in-venv

    source "$SCRIPT_DIR"/frontend-versions

    FRONTEND_LOCATION="${FRONTEND_LOCATION:-$VIRTUAL_ENV/var/hcsos}"
    FRONTEND_EDGE="$FRONTEND_LOCATION"/edge
    FRONTEND_API_PROXY="$FRONTEND_LOCATION"/api-proxy

    rm -rf "$FRONTEND_LOCATION" || :
    mkdir -p "$FRONTEND_LOCATION"
    mkdir -p "$FRONTEND_EDGE"
    mkdir -p "$FRONTEND_API_PROXY"

    echo "Installing Web Frontend (HAPI Edge)..." >&2
    git clone --depth=1 --branch="$HAPI_EDGE_VERSION" ssh://git@github.com/cschcs/hapi-edge-poc "$FRONTEND_EDGE"
    (
        cd "$FRONTEND_EDGE"
        npm install
        gulp deploy
    )
    RESULT=$?
    [ $RESULT -eq 0 ] || return $RESULT

    FRONTEND_INSTALL_DIR="$(mktemp -d -t frontend.XXXXXXXXXX)"
    (
        cd "$FRONTEND_INSTALL_DIR"
        git clone --depth=1 --branch="$MAIN_APP_VERSION" ssh://git@github.com/cschcs/ng-main
        git clone --depth=1 --branch="$AGILITY_X_VERSION" ssh://git@github.com/cschcs/ng-marketplace

        rsync -ar ng-main/dist/ngmain.js "$FRONTEND_EDGE"/public/app/
        rsync -ar ng-main/dist/themes/"$UI_THEME".js "$FRONTEND_EDGE"/public/app/theme.js
        rsync -ar ng-marketplace/distribute/assets/js/app.js "$FRONTEND_EDGE"/public/app/ngmarketplace.js

        rsync -ar ng-marketplace/distribute/assets/ "$FRONTEND_EDGE"/public/assets/
        rsync -ar ng-marketplace/distribute/fonts/ "$FRONTEND_EDGE"/public/fonts/
        rsync -ar ng-marketplace/distribute/i18n/ "$FRONTEND_EDGE"/public/i18n/

        rsync -ar "$FRONTEND_EDGE"/node_modules/ui-theme/dist/theme/ "$FRONTEND_EDGE"/public/theme/
    )
    RESULT=$?
    rm -rf "$FRONTEND_INSTALL_DIR"
    [ $RESULT -eq 0 ] || return $RESULT


    echo "Installing Frontend API Proxy..." >&2
    git clone --depth=1 --branch="$API_PROXY_VERSION" ssh://git@github.com/cschcs/api-proxy "$FRONTEND_API_PROXY"
    (
        cd "$FRONTEND_API_PROXY"
        npm install
    )
    RESULT=$?
    [ $RESULT -eq 0 ] || return $RESULT
}

#
# $1 - Setup 'dev' or 'prod'
# $2 - PyHome name (optional, defaults to branch name)
#
function setup-common()
{
    set -eEu

    assert-not-root

    install-prerequisites
    install-virtualenv
    install-pyhome "${2:-}"
    configure-bash-profile

    activate-venv

    configure-macports
    configure-installer
    install-binary-packages

    configure-module-requirements 'venv'
    configure-module-requirements "$1"

    configure-nodeenv

    install-frontend

    deactivate-venv
}

function setup-db()
{
    set -eEu
    activate-venv
    hcsos_pg_create "${1:-}" "${DEMO_DATA:-$VIRTUAL_ENV/etc/demo_data.json}"
    deactivate-venv

}
