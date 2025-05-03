# Exception Filter

An exception-filter is a process that handles specific errors.
This concept is based on [NestJS](https://docs.nestjs.com/exception-filters). However, in this implementation, exception-filter simply returns an HTTP response without content.
By default, it returns a status code of 204. To return a different status code, use `/:status` as the HTTP status code value.
For example: `http://127.0.0.1:10030/500`

## Example in Nginx

Instead of using Nginx's default error page, you can implement exception-filter in Nginx as follows

```
# /etc/nginx/.../status-code-error.conf
error_page 500 501 504 505 506 507 508 510 511 = @err_500;
```

When using directive [error_page](https://nginx.org/en/docs/http/ngx_http_core_module.html#error_page), http status codes 500 through 511 will use the error location named `@err_500`.
The next step is to create a proxy pass configuration that points to the exceptipn-filter URL

```
# /etc/nginx/.../catch-error.conf
location @err_500 {
    internal;

    rewrite ^ /500$1 break;
    # exception-filter service
    proxy_pass http://127.0.0.1:10030;
    proxy_set_header Host $host;

    include /etc/nginx/.../proxy.conf;
}
```

The location name `@err_500` forwards error processing to `http://127.0.0.1:10030`, which serves as the exception-filter URL.
Then, simply include it in the domain configuration section below

```conf
# /etc/nginx/.../domain.conf
server {
    listen 443 ssl;
    listen [::]:443 ssl;
    http2 on;

    server_name <DOMAIN>;

    include /etc/nginx/.../status-code-error.conf;

    ssl_certificate ...;
    ssl_certificate_key ...;
    # Optional
    # ssl_trusted_certificate ...;

    # include default headers

    location / {
        proxy_pass http://<APP>;
        proxy_set_header Host $host;
        include /etc/nginx/.../proxy.conf;
    }

    include /etc/nginx/.../catch-error.conf;
}
# For 3xx redirects
server {
    listen 80;
    listen [::]:80;

    server_name <DOMAIN>;

    include /etc/nginx/conf.d/redirect.conf;

    location / {
        return 301 https://<DOMAIN>$request_uri;
    }

    include /etc/nginx/conf.d/catch-redirect.conf;
}
```

```
# /etc/nginx/nginx.conf
...
...
...

http {
    charset utf-8;
    ...
    ...
    ...

    server {
        listen 80 default_server;
        listen [::]:80 default_server;
        listen 443 default_server ssl;
        listen [::]:443 default_server ssl;
        http2 on;

        server_name _;

        include /etc/nginx/.../status-code-error.conf;

        # This section is crucial
        # Create a dummy certificate using OpenSSL
        # This step aims to ensure that if there is access using the IP address directly, it will return a 444 error
        ssl_stapling off;
        ssl_certificate /etc/nginx/tls-dummy/fullchain.pem;
        ssl_certificate_key /etc/nginx/tls-dummy/cert-key.pem;

        include /etc/nginx/.../catch-error.conf;

        return 444;
    }

    include /etc/nginx/.../domain.conf
}
```

In the above configuration example, if your Nginx returns a 5xx error, it will use the exception-filter to return an error response.
You can also add 4xx errors in the same way as the 5xx error example above.

> [!NOTE]
>
> Make sure to use the latest version of Nginx.

## Build and Usage

```sh
gcc -o nob nob.c
./nob
```

There is a list of options available:

| Option         | Default Value | Description                                                       |
| -------------- | ------------- | ----------------------------------------------------------------- |
| `-h`, `--help` |               | Display help message                                              |
| `--max-conn`   | 16            | Specify backlog queue size for the socket                         |
| `--max-queue`  | 256           | Specify task in queue in thread pool                              |
| `--max-thread` | CPU(s)        | Specify worker threads that can run simultaneously in thread pool |
| `--port`       | 10030         | Specify exception-filter port                                     |

```sh
out/exception-filter --port <PORT>
```

## Formatter

`.clang-format` is based on [Google](https://google.github.io/styleguide/cppguide.html) style guide

```
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 120
AlignArrayOfStructures: Left
AlignAfterOpenBracket: Align
BracedInitializerIndentWidth: 4
```
