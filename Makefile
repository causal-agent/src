PREFIX ?= ~/.local
MANDIR ?= ${PREFIX}/share/man

MAN += adler32.3
MAN += adler32_combine.3
MAN += compress.3
MAN += compressBound.3
MAN += crc32.3
MAN += crc32_combine.3
MAN += deflate.3
MAN += deflateBound.3
MAN += deflateCopy.3
MAN += deflateEnd.3
MAN += deflateGetDictionary.3
MAN += deflateInit.3
MAN += deflateInit2.3
MAN += deflateParams.3
MAN += deflatePending.3
MAN += deflatePrime.3
MAN += deflateReset.3
MAN += deflateSetDictionary.3
MAN += deflateSetHeader.3
MAN += deflateTune.3
MAN += gzbuffer.3
MAN += gzclose.3
MAN += gzdirect.3
MAN += gzeof.3
MAN += gzerror.3
MAN += gzflush.3
MAN += gzfread.3
MAN += gzfwrite.3
MAN += gzgetc.3
MAN += gzgets.3
MAN += gzoffset.3
MAN += gzopen.3
MAN += gzprintf.3
MAN += gzputc.3
MAN += gzputs.3
MAN += gzread.3
MAN += gzseek.3
MAN += gzsetparams.3
MAN += gzungetc.3
MAN += gzwrite.3
MAN += inflate.3
MAN += inflateBack.3
MAN += inflateBackEnd.3
MAN += inflateBackInit.3
MAN += inflateCopy.3
MAN += inflateEnd.3
MAN += inflateGetDictionary.3
MAN += inflateGetHeader.3
MAN += inflateInit.3
MAN += inflateInit2.3
MAN += inflateMark.3
MAN += inflatePrime.3
MAN += inflateReset.3
MAN += inflateSetDictionary.3
MAN += inflateSync.3
MAN += uncompress.3
MAN += zlibCompileFlags.3
MAN += zlibVersion.3

MLINKS += adler32.3 adler32_z.3
MLINKS += compress.3 compress2.3
MLINKS += crc32.3 crc32_z.3
MLINKS += gzclose.3 gzclose_r.3
MLINKS += gzclose.3 gzclose_w.3
MLINKS += gzerror.3 gzclearerr.3
MLINKS += gzopen.3 gzdopen.3
MLINKS += gzseek.3 gzrewind.3
MLINKS += gzseek.3 gztell.3
MLINKS += inflateReset.3 inflateReset2.3
MLINKS += uncompress.3 uncompress2.3

lint:
	mandoc -T lint ${MAN} | grep -v 'referenced manual not found'

install:
	install -d ${MANDIR}/man3
	install -m 644 ${MAN} ${MANDIR}/man3
	set -- ${MLINKS}; while [ -n "$$*" ]; do \
		ln -fs $$1 ${MANDIR}/man3/$$2; shift 2; done

uninstall:
	rm -f ${MAN:%=${MANDIR}/man3/%}
	set -- ${MLINKS}; while [ -n "$$*" ]; do \
		rm -f ${MANDIR}/man3/$$2; shift 2; done
