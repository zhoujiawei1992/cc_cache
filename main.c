#include <signal.h>
#include <unistd.h>
#include "cc_http/cc_http_worker.h"
#include "cc_net/cc_connector.h"
#include "cc_net/cc_net.h"
#include "cc_util/cc_buffer.h"
#include "cc_util/cc_util.h"
#include "cc_worker/cc_worker.h"

#include "third/http_parser/http_parser.h"

int cc_test_http_parser_on_url(http_parser* parser, const char* at, size_t length) {
  debug_log("cc_test_http_parser_on_url, parser=%x, at=%x, length=%ld", parser, at, length);
  return 0;
}
int cc_test_http_parser_on_header_field(http_parser* parser, const char* at, size_t length) {
  debug_log("cc_test_http_parser_on_header_field, parser=%x, at=%x, length=%ld", parser, at, length);
  return 0;
}
int cc_test_http_parser_on_header_value(http_parser* parser, const char* at, size_t length) {
  debug_log("cc_test_http_parser_on_header_value, parser=%x, at=%x, length=%ld", parser, at, length);
  return 0;
}
int cc_test_http_parser_on_headers_complete(http_parser* parser) {
  debug_log("cc_test_http_parser_on_headers_complete, parser=%x", parser);
  return 0;
}
int cc_test_http_parser_on_body(http_parser* parser, const char* at, size_t length) {
  debug_log("cc_test_http_parser_on_body, parser=%x, at=%x, length=%ld", parser, at, length);
  return 0;
}
int cc_test_http_parser_on_message_complete(http_parser* parser) {
  debug_log("cc_test_http_parser_on_message_complete, parser=%x", parser);
  return 0;
}

int cc_test_http_parser_on_chunk_header(http_parser* parser) {
  debug_log("cc_test_http_parser_on_chunk_header, parser=%x, chunk_size=%uz", parser, parser->content_length);
  return 0;
}

int cc_test_http_parser_on_chunk_complete(http_parser* parser) {
  debug_log("cc_test_http_parser_on_chunk_complete, parser=%x, chunk_size=%uz", parser, parser->content_length);
  return 0;
}

int cc_test_http_parser_on_message_begin(http_parser* parser) {
  debug_log("cc_test_http_parser_on_message_begin, parser=%x", parser);
  return 0;
}

void test_parse() {
  static http_parser_settings settings = {
      cc_test_http_parser_on_message_begin,
      cc_test_http_parser_on_url,
      NULL,
      cc_test_http_parser_on_header_field,
      cc_test_http_parser_on_header_value,
      cc_test_http_parser_on_headers_complete,
      cc_test_http_parser_on_body,
      cc_test_http_parser_on_message_complete,
      cc_test_http_parser_on_chunk_header,
      cc_test_http_parser_on_chunk_complete,
  };
  http_parser parser;
  http_parser_init(&parser, HTTP_REQUEST);
  char buffer[] =
      "POST /1.txt HTTP/1.1\r\nHost: 127.0.0.1:8080\r\nUser-Agent: curl/7.61.1\r\nAccept: */*\r\nContent-Type: "
      "application/octet-stream\r\nTransfer-Encoding: chunked\r\n\r\n9\r\n123456789\r\n0\r\n\r\n";
  size_t length = sizeof(buffer) - 1;
  cc_string_t str = {buffer, length};
  debug_log("test_parse, content=\r\n%V", &str);
  size_t size = http_parser_execute(&parser, &settings, buffer, length);
  debug_log("test_parse, parser=%x, errno=%s, size=%uz", &parser, http_errno_name(parser.http_errno), size);
}

int main(int argc, char* argv[]) {
#ifdef CC_MEMCHECK
  memcheck_init();
#endif

  signal(SIGPIPE, SIG_IGN);
  error_log_open(&global_log_ctx, NULL, LOG_DEBUG);
  cc_worker_t* worker = cc_http_worker_create(CC_HTTP_WORKER);
  cc_worker_t* worker1 = cc_http_worker_create(CC_HTTP_ADMIN_WORKER);
  cc_start_worker(worker);
  cc_start_worker(worker1);
  cc_join_worker(worker);
  cc_join_worker(worker1);
  // test_parse();
  error_log_close(&global_log_ctx);

  const char* buffer = "";

  return 0;
}