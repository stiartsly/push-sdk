/*
 * Copyright (c) 2021 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>
#include <getopt.h>

#include <push.h>

static void usage(void)
{
    printf("Push Tool, an CLI tool to simplify push server configuration.\n");
    printf("Usage: pushtool [OPTION] register|unregister SERVER_HOST SERVER_PORT SCOPE fcm|apns "
           "PROJECT_ID|CERT_PATH API_KEY|KEY_PATH\n");
    printf("\n");
    printf("Debugging options:\n");
    printf("      --debug               Wait for debugger attach after start.\n");
    printf("\n");
}

typedef struct {
    char *op;
    push_server_t server;
    char *scope;
    char *psp;
    union {
        struct {
            char *prj_id;
            char *api_key;
        } fcm;
        struct {
            char *cert_path;
            char *key_path;
        } apns;
    } psp_specific;
} args_t;

#define NON_OPT_CNT (7)
int main(int argc, char *argv[])
{
    int wait_for_attach = 0;
    int rc;
    int (*op)(const push_server_t *, const registered_data_t *);
    const args_t *args;
    union {
        registered_data_t data;
        registered_project_key_t fcm;
        registered_certificate_t apns;
    } psp;

    int opt;
    int idx;
    struct option options[] = {
        { "debug",          no_argument,        NULL, 5 },
        { "help",           no_argument,        NULL, 'h' },
        { NULL,             0,                  NULL, 0 }
    };

    while ((opt = getopt_long(argc, argv, "c:h?", options, &idx)) != -1) {
        switch (opt) {
        case 5:
            wait_for_attach = 1;
            break;

        case 'h':
        case '?':
        default:
            usage();
            exit(-1);
        }
    }

    if (wait_for_attach) {
        printf("Wait for debugger attaching, process id is: %d.\n", getpid());
#ifndef _MSC_VER
        printf("After debugger attached, press any key to continue......");
        getchar();
#else
        DebugBreak();
#endif
    }

    if (argc - optind != NON_OPT_CNT) {
        printf("Arguments counts are not correct.");
        return -1;
    }

    args = (args_t *)(argv + optind);
    op = !strcmp(args->op, "register") ? register_push_service : unregister_push_service;
    !strcmp(args->psp, "fcm") ? registered_project_key_init(&psp.fcm, args->scope,
                                                            args->psp_specific.fcm.prj_id,
                                                            args->psp_specific.fcm.api_key) :
                                registered_certificate_init(&psp.apns, args->scope,
                                                            args->psp_specific.apns.cert_path,
                                                            args->psp_specific.apns.key_path);

    rc = op(&args->server, &psp.data);
    printf("status: %d\n", rc);

    return 0;
}