/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Source code using the multi interface to download many
 * files, with a capped maximum amount of simultaneous transfers.
 * </DESC>
 * Written by Michael Wallner
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#  include <unistd.h>
#endif
#include <curl/multi.h>

#include <uci.h>

#define UCI_CONFIG_FILE "/etc/config/4ipnet"
static struct uci_context * ctx = NULL;

bool load_config()
{
    struct uci_package * pkg = NULL;
    struct uci_element *e;
    char *tmp;
    const char *value;

    ctx = uci_alloc_context(); // 申请一个UCI上下文.
    if (UCI_OK != uci_load(ctx, UCI_CONFIG_FILE, &pkg))
        goto cleanup; //如果打开UCI文件失败,则跳到末尾 清理 UCI 上下文.

    /*遍历UCI的每一个节*/
    uci_foreach_element(&pkg->sections, e)
    {
        struct uci_section *s = uci_to_section(e);
        printf("section s's type is %s.\n",s->type);
        if(!strcmp("meter",s->type)) //this section is a meter
        {
            printf("this seciton is a meter.\n");
            if (NULL != (value = uci_lookup_option_string(ctx, s, "modbus_id")))
            {
                    tmp = strdup(value); //如果您想持有该变量值，一定要拷贝一份。当 pkg销毁后value的内存会被释放。
                    printf("%s's modbus_id is %s.\n",s->e.name,value);
            }
            if (NULL != (value = uci_lookup_option_string(ctx, s, "num_attr")))
            {
                    tmp = strdup(value); //如果您想持有该变量值，一定要拷贝一份。当 pkg销毁后value的内存会被释放。
                    printf("%s's num_attr is %s.\n",s->e.name,value);
            }
        }
        // 如果您不确定是 string类型 可以先使用 uci_lookup_option() 函数得到Option 然后再判断.
        // Option 的类型有 UCI_TYPE_STRING 和 UCI_TYPE_LIST 两种.
    }
    uci_unload(ctx, pkg); // 释放 pkg
cleanup:
    uci_free_context(ctx);
    ctx = NULL;
}


static const char *urls[] = {
  "http://www.microsoft.com",
  "http://www.opensource.org",
  "http://www.google.com",
  "http://www.yahoo.com",
  "http://www.ibm.com",
  "http://www.mysql.com",
  "http://www.oracle.com",
  "http://www.ripe.net",
  "http://www.iana.org",
  "http://www.amazon.com",
  "http://www.netcraft.com",
  "http://www.heise.de",
  "http://www.chip.de",
  "http://www.ca.com",
  "http://www.cnet.com",
  "http://www.news.com",
  "http://www.cnn.com",
  "http://www.wikipedia.org",
  "http://www.dell.com",
  "http://www.hp.com",
  "http://www.cert.org",
  "http://www.mit.edu",
  "http://www.nist.gov",
  "http://www.ebay.com",
  "http://www.playstation.com",
  "http://www.uefa.com",
  "http://www.ieee.org",
  "http://www.apple.com",
  "http://www.symantec.com",
  "http://www.zdnet.com",
  "http://www.fujitsu.com",
  "http://www.supermicro.com",
  "http://www.hotmail.com",
  "http://www.ecma.com",
  "http://www.bbc.co.uk",
  "http://news.google.com",
  "http://www.foxnews.com",
  "http://www.msn.com",
  "http://www.wired.com",
  "http://www.sky.com",
  "http://www.usatoday.com",
  "http://www.cbs.com",
  "http://www.nbc.com",
  "http://slashdot.org",
  "http://www.bloglines.com",
  "http://www.techweb.com",
  "http://www.newslink.org",
  "http://www.un.org",
};

#define MAX 10 /* number of simultaneous transfers */
#define CNT sizeof(urls)/sizeof(char *) /* total number of transfers to do */

static size_t cb(char *d, size_t n, size_t l, void *p)
{
  /* take care of the data here, ignored in this example */
  (void)d;
  (void)p;
  return n*l;
}

static void init(CURLM *cm, int i)
{
  CURL *eh = curl_easy_init();

  curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
  curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
  curl_easy_setopt(eh, CURLOPT_URL, urls[i]);
  curl_easy_setopt(eh, CURLOPT_PRIVATE, urls[i]);
  curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);

  curl_multi_add_handle(cm, eh);
}

int main(void)
{
  CURLM *cm;
  CURLMsg *msg;
  long L;
  unsigned int C = 0;
  int M, Q, U = -1;
  fd_set R, W, E;
  struct timeval T;

  curl_global_init(CURL_GLOBAL_ALL);

  cm = curl_multi_init();

  /* we can optionally limit the total amount of connections this multi handle
     uses */
  curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)MAX);

  for(C = 0; C < MAX; ++C) {
    init(cm, C);
  }

  while(U) {
    curl_multi_perform(cm, &U);

    if(U) {
      FD_ZERO(&R);
      FD_ZERO(&W);
      FD_ZERO(&E);

      if(curl_multi_fdset(cm, &R, &W, &E, &M)) {
        fprintf(stderr, "E: curl_multi_fdset\n");
        return EXIT_FAILURE;
      }

      if(curl_multi_timeout(cm, &L)) {
        fprintf(stderr, "E: curl_multi_timeout\n");
        return EXIT_FAILURE;
      }
      if(L == -1)
        L = 100;

      if(M == -1) {
#ifdef WIN32
        Sleep(L);
#else
        sleep((unsigned int)L / 1000);
#endif
      }
      else {
        T.tv_sec = L/1000;
        T.tv_usec = (L%1000)*1000;

        if(0 > select(M + 1, &R, &W, &E, &T)) {
          fprintf(stderr, "E: select(%i,,,,%li): %i: %s\n",
              M + 1, L, errno, strerror(errno));
          return EXIT_FAILURE;
        }
      }
    }

    while((msg = curl_multi_info_read(cm, &Q))) {
      if(msg->msg == CURLMSG_DONE) {
        char *url;
        CURL *e = msg->easy_handle;
        curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
        fprintf(stderr, "R: %d - %s <%s>\n",
                msg->data.result, curl_easy_strerror(msg->data.result), url);
        curl_multi_remove_handle(cm, e);
        curl_easy_cleanup(e);
      }
      else {
        fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
      }
      if(C < CNT) {
        init(cm, C++);
        U++; /* just to prevent it from remaining at 0 if there are more
                URLs to get */
      }
    }
  }

  curl_multi_cleanup(cm);
  curl_global_cleanup();

  return EXIT_SUCCESS;
}
