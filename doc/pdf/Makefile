PDFS += abi.pdf
PDFS += c11.pdf
PDFS += elf.pdf
PDFS += intel-64-opt.pdf
PDFS += intel-64-sdm-vol-1.pdf
PDFS += intel-64-sdm-vol-2.pdf
PDFS += intel-64-sdm-vol-3.pdf
PDFS += intel-64-sdm-vol-4.pdf
PDFS += multiboot.pdf

ELF = http://refspecs.linuxbase.org/elf
INTEL = https://software.intel.com/sites/default/files/managed

URL_abi.pdf = ${ELF}/x64_64-abi-0.99.pdf
URL_c11.pdf = http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
URL_elf.pdf = ${ELF}/elf.pdf
URL_intel-64-opt.pdf = ${INTEL}/9e/bc/64-ia-32-architectures-optimization-manual.pdf
URL_intel-64-sdm-vol-1.pdf = ${INTEL}/a4/60/253665-sdm-vol-1.pdf
URL_intel-64-sdm-vol-2.pdf = ${INTEL}/a4/60/325383-sdm-vol-2abcd.pdf
URL_intel-64-sdm-vol-3.pdf = ${INTEL}/a4/60/325384-sdm-vol-3abcd.pdf
URL_intel-64-sdm-vol-4.pdf = ${INTEL}/22/0d/335592-sdm-vol-4.pdf
URL_multiboot.pdf = https://www.gnu.org/software/grub/manual/multiboot/multiboot.pdf

all: ${PDFS}

${PDFS}:
	curl -o $@ ${URL_$@}
	chmod 444 $@

clean:
	rm -f ${PDFS}
