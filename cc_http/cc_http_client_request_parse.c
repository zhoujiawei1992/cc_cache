#include "cc_http/cc_http_client_request.h"

#include <string.h>
int cc_http_parser_on_url(http_parser* parser, const char* at, size_t length) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  http_context->http_request.url.data = at;
  http_context->http_request.url.size = length;
  debug_log("cc_http_parser_on_url, parser=%x, http_context=%x, url=%V, method=%s", parser, http_context,
            &http_context->http_request.url, http_method_str(parser->method));
  return 0;
}
int cc_http_parser_on_header_field(http_parser* parser, const char* at, size_t length) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  http_context->http_request.parsing_header_field.data = at;
  http_context->http_request.parsing_header_field.size = length;
  debug_log("cc_http_parser_on_header_field, parser=%x, http_context=%x, parsing_header_field=%V", parser, http_context,
            &http_context->http_request.parsing_header_field);
  return 0;
}
int cc_http_parser_on_header_value(http_parser* parser, const char* at, size_t length) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  cc_http_header_t* header = (cc_http_header_t*)cc_malloc(sizeof(cc_http_header_t));
  if (header == NULL) {
    error_log("cc_http_parser_on_header_value, malloc error");
    return -1;
  }
  header->field = http_context->http_request.parsing_header_field;
  header->value.data = at;
  header->value.size = length;
  cc_slist_insert(&http_context->http_request.header_list, &header->node);

  if (strncasecmp(header->field.data, g_content_length_header_field.data, header->field.size) == 0) {
    unsigned long long content_length = 0;
    if (str2ull(header->value.data, header->value.size, &content_length) == 0) {
      http_context->http_request.content_length = (size_t)content_length;
    } else {
      error_log("cc_http_parser_on_header_value, parser header=%V failed, value=%V", &header->field, &header->value);
      return -1;
    }
  }

  if (strncasecmp(header->field.data, g_transfer_encoding_chunked_field.data, header->field.size) == 0 &&
      strncasecmp(header->value.data, g_transfer_encoding_chunked_value.data, header->value.size) == 0) {
    http_context->http_request.chunk_header = 1;
  }

  http_context->http_request.header_size = at + length + 2 - http_context->header_buffer->data;
  debug_log("cc_http_parser_on_header_value, parser=%x, http_context=%x, field=%V, value=%V, header_size=%d", parser,
            http_context, &header->field, &header->value, http_context->http_request.header_size);
  return 0;
}
int cc_http_parser_on_headers_complete(http_parser* parser) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  http_context->http_state = HTTP_STATE_REQUEST_HEADER_DONE;
  http_context->http_request.header_size = http_context->http_request.header_size + 2;
  debug_log(
      "cc_http_parser_on_headers_complete, parser=%x, http_context=%x, http_state=%d, header_size=%d, body_size=%d, "
      "bytes_recv=%d, keepalive=%d, chunk_header=%ld",
      parser, http_context, http_context->http_state, http_context->http_request.header_size,
      http_context->header_buffer->size - http_context->http_request.header_size, http_context->http_request.bytes_recv,
      http_should_keep_alive(parser), http_context->http_request.chunk_header);
  if (http_context->on_header_done_proc) {
    http_context->on_header_done_proc(http_context);
  }
  return 0;
}
int cc_http_parser_on_body(http_parser* parser, const char* at, size_t length) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  debug_log(
      "cc_http_parser_on_body, parser=%x, http_context=%x, content_length=%d, header_size=%d, body_size=%d, "
      "bytes_recv=%d, chunk_header=%ld",
      parser, http_context, http_context->http_request.content_length, http_context->http_request.header_size,
      http_context->http_request.bytes_recv - http_context->http_request.header_size,
      http_context->http_request.bytes_recv, http_context->http_request.chunk_header);
  if (http_context->on_body_proc) {
    http_context->on_body_proc(http_context, at, length);
  }
  return 0;
}
int cc_http_parser_on_message_complete(http_parser* parser) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  http_context->http_state = HTTP_STATE_REQUEST_BODY_DONE;
  http_context->http_request.chunk_complete = 1;
  debug_log(
      "cc_http_parser_on_message_complete, parser=%x, http_context=%x, content_length=%d, header_size=%d, "
      "body_size=%d, bytes_recv=%d, keepalive=%d, chunk_header=%ld, chunk_complete=%ld",
      parser, http_context, http_context->http_request.content_length, http_context->http_request.header_size,
      http_context->http_request.bytes_recv - http_context->http_request.header_size,
      http_context->http_request.bytes_recv, http_should_keep_alive(parser), http_context->http_request.chunk_header,
      http_context->http_request.chunk_complete);
  if (http_context->on_message_done_proc) {
    http_context->on_message_done_proc(http_context);
  }
  return 0;
}
/*
int cc_http_parser_on_chunk_header(http_parser* parser) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  debug_log("cc_http_parser_on_chunk_header, parser=%x, http_context=%x, chunk_size=%d", parser, http_context,
            parser->content_length, parser->flags & F_CHUNKED);
  if (http_context->on_chunk_header) {
    http_context->on_chunk_header(http_context);
  }
}
int cc_http_parser_on_chunk_complete(http_parser* parser) {
  cc_http_context_t* http_context = (cc_http_context_t*)parser->data;
  debug_log("cc_http_parser_on_chunk_complete, parser=%x, http_context=%x, chunk_size=%d", parser, http_context,
            parser->content_length, parser->flags & F_CHUNKED);
  if (http_context->on_chunk_complete) {
    http_context->on_chunk_complete(http_context);
  }
}*/

int cc_http_client_execute_parse(cc_http_context_t* http_context, const char* buffer, unsigned int length,
                                 size_t* nbytes) {
  static http_parser_settings settings = {
      NULL,
      cc_http_parser_on_url,
      NULL,
      cc_http_parser_on_header_field,
      cc_http_parser_on_header_value,
      cc_http_parser_on_headers_complete,
      cc_http_parser_on_body,
      cc_http_parser_on_message_complete,
      NULL,  // cc_http_parser_on_chunk_header,
      NULL,  // cc_http_parser_on_chunk_complete,
  };

  if (http_context->http_state == HTTP_STATE_REQUEST_WAIT) {
    http_context->http_parser.data = http_context;
    http_parser_init(&http_context->http_parser, HTTP_REQUEST);
    http_context->http_state = HTTP_STATE_REQUEST_HEADER_WAIT;
    cc_http_client_reset_request(http_context, &http_context->http_request);
  }

  if (http_context->http_state == HTTP_STATE_REQUEST_HEADER_WAIT) {
    http_context->http_request.bytes_recv += length;
    *nbytes = http_parser_execute(&http_context->http_parser, &settings, buffer, length);
    if (HTTP_PARSER_ERRNO(&http_context->http_parser) != HPE_OK) {
      error_log(
          "cc_http_client_execute_parse, http_parser_execute failed, parser=%x, http_context=%x, errno=%s, error=%s",
          &http_context->http_parser, http_context, http_errno_name(http_context->http_parser.http_errno),
          http_errno_description(http_context->http_parser.http_errno));
      return -1;
    }
    debug_log("cc_http_client_execute_parse, http_parser_execute, parser=%x, http_context=%x, nbytes=%uz",
              &http_context->http_parser, http_context, *nbytes);
  } else if (http_context->http_state == HTTP_STATE_REQUEST_HEADER_DONE) {
    http_context->http_request.bytes_recv += length;
    *nbytes = http_parser_execute(&http_context->http_parser, &settings, buffer, length);
    if (HTTP_PARSER_ERRNO(&http_context->http_parser) != HPE_OK) {
      error_log(
          "cc_http_client_execute_parse, http_parser_execute failed, parser=%x, http_context=%x, errno=%s, error=%s",
          &http_context->http_parser, http_context, http_errno_name(http_context->http_parser.http_errno),
          http_errno_description(http_context->http_parser.http_errno));
      return -1;
    }
    debug_log("cc_http_client_execute_parse, http_parser_execute, parser=%x, http_context=%x, nbytes=%uz",
              &http_context->http_parser, http_context, *nbytes);
  }
  return 0;
}