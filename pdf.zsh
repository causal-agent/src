#!/usr/bin/env zsh
set -o errexit -o nounset -o pipefail

mkdir -p pdf

fetch() {
	[[ -f "pdf/$1" ]] && return
	curl --silent --show-error --output "pdf/$1" "$2"
	echo "pdf/$1"
}

elf() {
	fetch "$1" "http://refspecs.linuxbase.org/elf/$2"
}
intel() {
	fetch "$1" "https://software.intel.com/sites/default/files/managed/$2"
}

elf abi.pdf x86_64-abi-0.99.pdf
elf elf.pdf elf.pdf
fetch multiboot.pdf 'https://www.gnu.org/software/grub/manual/multiboot/multiboot.pdf'
intel intel-64-opt.pdf 9e/bc/64-ia-32-architectures-optimization-manual.pdf
intel intel-64-sdm-vol-1.pdf a4/60/253665-sdm-vol-1.pdf
intel intel-64-sdm-vol-2.pdf a4/60/325383-sdm-vol-2abcd.pdf
intel intel-64-sdm-vol-3.pdf a4/60/325384-sdm-vol-3abcd.pdf
intel intel-64-sdm-vol-4.pdf 22/0d/335592-sdm-vol-4.pdf

chmod 444 pdf/*.pdf
