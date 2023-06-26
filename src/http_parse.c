
/*
 * Copyright (C) Zhu Jiashun
 * Copyright (C) Zaver
 */
#include <bits/types.h>

#include "http.h"
#include "http_parse.h"
#include "error.h"
#include "memory_pool.h"

extern zlog_category_t *g_zc;

/**
 * @brief 处理http请求对象 r的请求体
 * 
 * @param r http请求对象
 * @return int
 *  
 */
int zv_http_parse_request_line(zv_http_request_t *r) {
    unsigned char ch, *p, *m;
    size_t pi;

    enum {
        sw_start = 0,
        sw_method,
        sw_spaces_before_uri,
        sw_after_slash_in_uri,
        sw_http,
        sw_http_H,
        sw_http_HT,
        sw_http_HTT,
        sw_http_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_spaces_after_digit,
        sw_almost_done
    } state;

    state = r->state; // r->state初始值为0

    // r->pos 缓冲区开始的位置
    // r->last 缓冲区最后的位置 左闭右开
    // log_info("ready to parese request line, start = %d, last= %d", (int)r->pos, (int)r->last);
    for (pi = r->pos; pi < r->last; pi++) {
        // p = (u_char *)&r->buf[pi % MAX_BUF]; // p 指向待处理字符
        p = (unsigned char *)&r->buf[pi % MAX_BUF]; // p 指向待处理字符
        
        ch = *p;  

        switch (state) {

        /* HTTP methods: GET, HEAD, POST */
        case sw_start:
            r->request_start = p;

            if (ch == CR || ch == LF) {
                break; // 跳出switch 后面执行下次循环
            }

            // 如果首字母不是大写字符A-Z 也不是 _ 返回 请求格式有问题
            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return ZV_HTTP_PARSE_INVALID_METHOD;
            }

            // 如果首字母不是大写字符A-Z 说明符合method的格式，状态修改为sw_method 用以解析method
            state = sw_method;
            break;

        case sw_method:
            // method读完
            if (ch == ' ') {
                
                r->method_end = p; // 修改method_end的值
                m = r->request_start;// 获取r->request_start 的值

                // 根据methond字符串的长度判断接下来的处理流程
                switch (p - m) {

                case 3:
                    if (zv_str3_cmp(m, 'G', 'E', 'T', ' ')) {
                        r->method = ZV_HTTP_GET; // 修改r->method 为ZV_HTTP_GET
                        break;
                    }

                    break;

                case 4:
                    if (zv_str3Ocmp(m, 'P', 'O', 'S', 'T')) {
                        r->method = ZV_HTTP_POST;
                        break;
                    }

                    if (zv_str4cmp(m, 'H', 'E', 'A', 'D')) {
                        r->method = ZV_HTTP_HEAD;
                        break;
                    }

                    break;
                default:
                    r->method = ZV_HTTP_UNKNOWN;
                    break;
                }
                // 修改状态 判断url部分
                state = sw_spaces_before_uri;
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_') {
                return ZV_HTTP_PARSE_INVALID_METHOD;
            }

            break;

        /* space* before URI */
        case sw_spaces_before_uri:

            // url部分第一个字符为 / 
            if (ch == '/') {
                r->uri_start = p;
                state = sw_after_slash_in_uri; // 将状态设为sw_after_slash_in_uri处理/后面的内容
                break;
            }

            // url部分第一个字符不为 / 
            switch (ch) { 
                case ' ': //如果是空格继续读
                    break;
                default: // 如果不是 / 和 空格
                    return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break; 

        case sw_after_slash_in_uri:
            // 遇到空格 代表uri 读取完
            switch (ch) {
            case ' ':
                r->uri_end = p; // 设置uri_end 
                state = sw_http; // 更新state
                break;
            default:
                break;
            }
            break;

        /* space+ after URI */
        case sw_http:
            switch (ch) {
            case ' ':
                break;
            case 'H':
                state = sw_http_H;
                break;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_H:
            switch (ch) {
            case 'T':
                state = sw_http_HT;
                break;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HT:
            switch (ch) {
            case 'T':
                state = sw_http_HTT;
                break;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HTT:
            switch (ch) {
            case 'P':
                state = sw_http_HTTP;
                break;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        case sw_http_HTTP:
            switch (ch) {
            case '/':
                state = sw_first_major_digit; // HTTP核对完 核对/ 
                break;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        /* first digit of major HTTP version */
        case sw_first_major_digit:
            if (ch < '1' || ch > '9') {
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }

            r->http_major = ch - '0'; // 修改http_major 1
            state = sw_major_digit;
            break;

        /* major HTTP version or dot */
        case sw_major_digit:
            if (ch == '.') {
                state = sw_first_minor_digit; // 读到 . 
                break;
            }

            if (ch < '0' || ch > '9') {
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }

            // 没有读到 . 
            r->http_major = r->http_major * 10 + ch - '0'; 
            break;

        /* first digit of minor HTTP version */
        case sw_first_minor_digit:
            if (ch < '0' || ch > '9') {
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }

            r->http_minor = ch - '0';
            state = sw_minor_digit;
            break;

        /* minor HTTP version or end of request line */
        case sw_minor_digit:
            if (ch == CR) { // 读到\r
                state = sw_almost_done;
                break;
            }

            if (ch == LF) { // 读到\n
                goto done;
            }

            if (ch == ' ') { // 读到' '
                state = sw_spaces_after_digit;
                break;
            }

            if (ch < '0' || ch > '9') {
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }

            // 0-9的数字
            r->http_minor = r->http_minor * 10 + ch - '0';
            break;

        case sw_spaces_after_digit: // 小数点后面的数字
            switch (ch) {
            case ' ':
                break;
            case CR:
                state = sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
            break;

        /* end of request line */
        case sw_almost_done:
            r->request_end = p - 1; // 请求行的最后一个字符位置 不包括\r\n
            switch (ch) {
            case LF:
                goto done;
            default:
                return ZV_HTTP_PARSE_INVALID_REQUEST;
            }
        }
    }

    // pi == r->last 时
    r->pos = pi;
    r->state = state;

    return ZV_AGAIN;

done:

    r->pos = pi + 1;

    if (r->request_end == NULL) {
        r->request_end = p;
    }

    r->state = sw_start;

    return ZV_OK;
}

int zv_http_parse_request_body(zv_http_request_t *r) {
    unsigned char ch, *p;
    size_t pi;

    enum {
        sw_start = 0,
        sw_key,
        sw_spaces_before_colon,
        sw_spaces_after_colon,
        sw_value,
        sw_cr,
        sw_crlf,
        sw_crlfcr
    } state;

    state = r->state; // 初始状态sw_start
    check(state == 0, "state should be 0");

    zlog_info(g_zc, "ready to parese request body, start = %d, last= %d", r->pos, r->last);

    zv_http_header_t *hd; 
    for (pi = r->pos; pi < r->last; pi++) {
        p = (unsigned char *)&r->buf[pi % MAX_BUF];
        ch = *p;

        switch (state) {
        case sw_start:
            if (ch == CR || ch == LF) {
                break; // 继续下一个字符判断
            }

            r->cur_header_key_start = p;
            state = sw_key;
            break;
        case sw_key: // 当前状态是处理key, 一直处理直到遇到' '或者 :
            if (ch == ' ') {
                r->cur_header_key_end = p;
                state = sw_spaces_before_colon;
                break;
            }

            if (ch == ':') {
                r->cur_header_key_end = p;
                state = sw_spaces_after_colon;
                break;
            }

            break;
        case sw_spaces_before_colon:// 当前状态是处理 : 前面的" ", 一直处理直到遇到 : 
            if (ch == ' ') {
                break; 
            } else if (ch == ':') {
                state = sw_spaces_after_colon;
                break;
            } else {
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }
        case sw_spaces_after_colon: // 当前状态是处理 : 后面的" ", 一直处理直到遇到 : 
            if (ch == ' ') {
                break;
            }

            state = sw_value;
            r->cur_header_value_start = p;
            break;
        case sw_value:  // 当前状态是处理 : 后面的value部分,  :
            if (ch == CR) {
                r->cur_header_value_end = p;
                state = sw_cr;
            }

            if (ch == LF) {
                r->cur_header_value_end = p;
                state = sw_crlf;
            }
            
            break;
        case sw_cr:
            if (ch == LF) { // 如果末尾是\r\n
                state = sw_crlf;
                // save the current http header
                hd = (zv_http_header_t *)malloc(sizeof(zv_http_header_t));
                hd->key_start   = r->cur_header_key_start;
                hd->key_end     = r->cur_header_key_end;
                hd->value_start = r->cur_header_value_start;
                hd->value_end   = r->cur_header_value_end;
                
                printf("&(hd->list) %p\n", &(hd->list));
                printf(" &(r->list) = %p\n",  &(r->list));
                zlog_info(g_zc, "lis_head");
                // 把hd->list 添加到r->list 后
                list_add(&(hd->list), &(r->list));
                zlog_info(g_zc, "lis_head");

                break;
            } else {
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }

        case sw_crlf:
            if (ch == CR) { // \n\r
                state = sw_crlfcr; 
            } else { // \n和其他字符
                r->cur_header_key_start = p;
                state = sw_key;
            }
            break;

        case sw_crlfcr: 
            switch (ch) {
            case LF: // \n\r\n
                goto done;
            default:
                return ZV_HTTP_PARSE_INVALID_HEADER;
            }
            break;
        }   
    }

    r->pos = pi;
    r->state = state;

    return ZV_AGAIN;

done:
    r->pos = pi + 1;

    r->state = sw_start;

    return ZV_OK;
}
