/* Copyright (C) 2020  June McEnroe <june@causal.agency>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#ifdef __FreeBSD__
#include <sys/capsicum.h>
#endif

#include <kcgi.h>
#include <kcgihtml.h>

static int dir = -1;

static const char *Page = "up";
static const struct kvalid Key = { NULL, "file" };

static enum kcgi_err head(struct kreq *req, enum khttp http, enum kmime mime) {
	return khttp_head(req, kresps[KRESP_STATUS], "%s", khttps[http])
		|| khttp_head(req, kresps[KRESP_CONTENT_TYPE], "%s", kmimetypes[mime]);
}

static enum kcgi_err fail(struct kreq *req, enum khttp http) {
	return head(req, http, KMIME_TEXT_PLAIN)
		|| khttp_body(req)
		|| khttp_printf(req, "%s\n", khttps[http]);
}

static const char *upload(const char *ext, void *ptr, size_t len) {
	static char name[256];
	snprintf(
		name, sizeof(name), "%jx%08x%s%s",
		(intmax_t)time(NULL), arc4random(),
		(ext && ext[0] != '.' ? "." : ""), (ext ? ext : "")
	);
	int fd = openat(dir, name, O_CREAT | O_EXCL | O_WRONLY, 0644);
	if (fd < 0) {
		warn("%s", name);
		return NULL;
	}
	ssize_t n = write(fd, ptr, len);
	int error = close(fd);
	if (n < 0 || error) {
		warn("%s", name);
		return NULL;
	}
	return name;
}

static enum kcgi_err handle(struct kreq *req) {
	if (req->page) return fail(req, KHTTP_404);

	if (req->method == KMETHOD_GET) {
		struct khtmlreq html;
		struct khtmlreq *h = &html;
		return head(req, KHTTP_200, KMIME_TEXT_HTML)
			|| khttp_body(req)
			|| khtml_open(h, req, 0)
			|| khtml_elem(h, KELEM_DOCTYPE)
			|| khtml_elem(h, KELEM_TITLE)
			|| khtml_puts(h, "Upload")
			|| khtml_closeelem(h, 1)
			|| khtml_attr(
				h, KELEM_FORM,
				KATTR_METHOD, "post",
				KATTR_ACTION, "",
				KATTR_ENCTYPE, "multipart/form-data",
				KATTR__MAX
			)
			|| khtml_attr(
				h, KELEM_INPUT,
				KATTR_TYPE, "file",
				KATTR_NAME, Key.name,
				KATTR__MAX
			)
			|| khtml_attr(
				h, KELEM_INPUT,
				KATTR_TYPE, "submit",
				KATTR_VALUE, "Upload",
				KATTR__MAX
			)
			|| khtml_close(h);

	} else if (req->method == KMETHOD_POST) {
		struct kpair *field = req->fieldmap[0];
		if (!field || !field->valsz) return fail(req, KHTTP_400);

		const char *ext = strrchr(field->file, '.');
		const char *name = upload(ext, field->val, field->valsz);
		if (!name) return fail(req, KHTTP_507);

		return head(req, KHTTP_303, KMIME_TEXT_PLAIN)
			|| khttp_head(req, kresps[KRESP_LOCATION], "/%s", name)
			|| khttp_body(req)
			|| khttp_puts(req, name);

	} else if (req->method == KMETHOD_PUT) {
		struct kpair *field = req->fields;
		if (!field || !field->valsz) return fail(req, KHTTP_400);

		const char *ext = req->suffix;
		if (!ext[0]) ext = strrchr(field->file, '.');
		const char *name = upload(ext, field->val, field->valsz);
		if (!name) return fail(req, KHTTP_507);

		return head(req, KHTTP_200, KMIME_TEXT_PLAIN)
			|| khttp_body(req)
			|| khttp_printf(
				req, "%s://%s/%s\n", kschemes[req->scheme], req->host, name
			);

	} else {
		return fail(req, KHTTP_405);
	}
}

static void cgi(void) {
#ifdef __OpenBSD__
	if (pledge("stdio wpath cpath proc", NULL)) err(EX_OSERR, "pledge");
#endif

	struct kreq req;
	enum kcgi_err error = khttp_parse(&req, &Key, 1, &Page, 1, 0);
	if (error) errx(EX_PROTOCOL, "khttp_parse: %s", kcgi_strerror(error));

#ifdef __OpenBSD__
	if (pledge("stdio wpath cpath", NULL)) err(EX_OSERR, "pledge");
#endif
#ifdef __FreeBSD__
	if (cap_enter()) err(EX_OSERR, "cap_enter");
#endif

	error = handle(&req);
	if (error) errx(EX_PROTOCOL, "%s", kcgi_strerror(error));
	khttp_free(&req);
}

static void fcgi(void) {
#ifdef __OpenBSD__
	if (pledge("stdio wpath cpath unix sendfd recvfd proc", NULL)) {
		err(EX_OSERR, "pledge");
	}
#endif

	struct kfcgi *fcgi;
	enum kcgi_err error = khttp_fcgi_init(&fcgi, &Key, 1, &Page, 1, 0);
	if (error) errx(EX_CONFIG, "khttp_fcgi_init: %s", kcgi_strerror(error));

#ifdef __OpenBSD__
	if (pledge("stdio wpath cpath recvfd", NULL)) err(EX_OSERR, "pledge");
#endif
#ifdef __FreeBSD__
	if (cap_enter()) err(EX_OSERR, "cap_enter");
#endif

	for (
		struct kreq req;
		KCGI_OK == (error = khttp_fcgi_parse(fcgi, &req));
		khttp_free(&req)
	) {
		error = handle(&req);
		if (error && error != KCGI_HUP) break;
	}
	if (error != KCGI_EXIT) {
		errx(EX_PROTOCOL, "khttp_fcgi_parse: %s", kcgi_strerror(error));
	}
	khttp_fcgi_free(fcgi);
}

int main(int argc, char *argv[]) {
	int error;
	const char *path = (argc > 1 ? argv[1] : ".");
	dir = open(path, O_DIRECTORY);
	if (dir < 0) err(EX_NOINPUT, "%s", path);

#ifdef __OpenBSD__
	error = unveil(path, "wc");
	if (error) err(EX_OSERR, "unveil");
#endif

#ifdef __FreeBSD__
	cap_rights_t rights;
	cap_rights_init(&rights, CAP_LOOKUP, CAP_CREATE, CAP_PWRITE);
	error = cap_rights_limit(dir, &rights);
	if (error) err(EX_OSERR, "cap_rights_limit");
#endif

	if (khttp_fcgi_test()) {
		fcgi();
	} else {
		cgi();
	}
}
