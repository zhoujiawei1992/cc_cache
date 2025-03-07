#include <signal.h>
#include <unistd.h>
#include "cc_http/cc_http_worker.h"
#include "cc_net/cc_connector.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_util.h"
#include "cc_worker/cc_worker.h"

int main(int argc, char* argv[]) {
  signal(SIGPIPE, SIG_IGN);
  error_log_open(&global_log_ctx, NULL, LOG_DEBUG);
  cc_worker_t* worker = cc_http_worker_create();
  cc_start_worker(worker);
  cc_join_worker(worker);
  error_log_close(&global_log_ctx);
  return 0;
}