#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

UpdateStatus updateStatus = UPDATE_IDLE;
char remoteVersion[32] = "";

static Result http_download(const char *url, u8 **out_buf, size_t *out_size) {
    Result ret = 0;
    httpcContext context;
    u32 status_code = 0;
    u32 contentsize = 0;
    u8 *buf = NULL;

    ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
    if (R_FAILED(ret)) return ret;

    // Set User-Agent as required by GitHub
    ret = httpcAddRequestHeaderField(&context, "User-Agent", "Tunez3DS-Updater");
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    ret = httpcBeginRequest(&context);
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    ret = httpcGetResponseStatusCode(&context, &status_code);
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    if (status_code != 200) {
        httpcCloseContext(&context);
        return -1;
    }

    ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
    if (R_FAILED(ret)) {
        httpcCloseContext(&context);
        return ret;
    }

    buf = (u8 *)malloc(contentsize + 1);
    if (!buf) {
        httpcCloseContext(&context);
        return -1;
    }
    memset(buf, 0, contentsize + 1);

    ret = httpcDownloadData(&context, buf, contentsize, NULL);
    if (R_FAILED(ret)) {
        free(buf);
        httpcCloseContext(&context);
        return ret;
    }

    *out_buf = buf;
    *out_size = contentsize;

    httpcCloseContext(&context);
    return 0;
}

void checkForUpdates(void) {
    updateStatus = UPDATE_CHECKING;
    
    u8 *buf = NULL;
    size_t size = 0;
    const char *url = "https://raw.githubusercontent.com/veylo-3DS/Tunez-3DS/main/version.txt";

    Result ret = http_download(url, &buf, &size);
    if (R_FAILED(ret)) {
        updateStatus = UPDATE_ERROR;
        return;
    }

    // Clean up the version string (remove newlines/spaces)
    char *v = (char *)buf;
    for (size_t i = 0; i < size; i++) {
        if (v[i] == '\r' || v[i] == '\n' || v[i] == ' ') {
            v[i] = '\0';
            break;
        }
    }

    strncpy(remoteVersion, v, sizeof(remoteVersion) - 1);
    remoteVersion[sizeof(remoteVersion) - 1] = '\0';

    if (strcmp(remoteVersion, APP_VERSION) != 0) {
        updateStatus = UPDATE_AVAILABLE;
    } else {
        updateStatus = UPDATE_UP_TO_DATE;
    }

    free(buf);
}
