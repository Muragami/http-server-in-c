#include "server.h"
#include "pthread_pool.h"
#include "tconfig.h"

// ----- HTTP server related declarations ----- //

extern struct Bstring *filename;
extern void handle_connection(int conn_fd);

static evutil_socket_t listener;
static struct sockaddr_in sin;
static struct event_base *base;
static struct event *listener_event;
struct TConfig *config = NULL;
static int port = 40000;
static int threads = 16;

void cleanup_and_exit()
{
  if (listener != -1)
  {
    close(listener);
  }

  if (filename != NULL)
  {
    bstring_free(filename);
  }
  exit(0);
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
  struct event_base *base = arg;
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, (struct sockaddr *)&ss, &slen);
  if (fd < 0)
  {
    perror("accept");
  }
  else if (fd > FD_SETSIZE)
  {
    close(fd);
  }
  else
  {
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    // bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
    // bufferevent_enable(bev, EV_READ | EV_WRITE);
  }
}

void read_config(const char *config_path)
{
  config = tconfig_new();
  if (tconfig_read_file(config, config_path) != 0)
  {
    fprintf(stderr, "Failed to read config file: %s\n", config_path);
    tconfig_free(config);
    return;
  }

  // get server config
  ini_table_get_entry_as_int(config, "server", "port", &port);
  ini_table_get_entry_as_int(config, "server", "threads", &threads);
}

int main(int argc, char **argv)
{
#ifdef _WIN32
  WSADATA WsaData;
  return (WSAStartup(MAKEWORD(2, 2), &WsaData) != NO_ERROR);
#endif
  read_config((const char *)argv[2]);
  base = event_base_new();
  if (!base)
    return; /*XXXerr*/

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(port);

  listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

#ifndef _WIN32
  {
    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  }
#endif

  if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("bind");
    return;
  }

  if (listen(listener, 16) < 0)
  {
    perror("listen");
    return;
  }

  listener_event = event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void *)base);
  event_add(listener_event, NULL);

  event_base_dispatch(base);

  cleanup_and_exit();
  return 0;
}
