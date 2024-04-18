/*
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static char *ok_header(size_t len, const char *content_type);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
static void serve_request(int fd, dictionary_t *query);
static dictionary_t *registerClient(dictionary_t *query, const char *user);
static void WriteResponse(int fd, char *body);
static char *UpdateUserFriends(char *newFriends, const char *user);
static char *getFriFriend(char *host, char *port, char *friends);
static void getFriends(int fd, dictionary_t *query);
static void beFriends(int fd, dictionary_t *query);
static void unFriend(int fd, dictionary_t *query);
static void introduceFriend(int fd, dictionary_t *query);
static char *getBody(dictionary_t *user_Friends_Dictionary);

pthread_mutex_t mutex;
static dictionary_t *AllClients;
void *thread_client(void *args);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  AllClients = make_dictionary(COMPARE_CASE_SENS, free);
  pthread_mutex_init(&mutex, NULL);

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1)
  {
    pthread_t tid;
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0)
    {
      Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      int *arg = malloc(sizeof(int));
      *arg = connfd;
      pthread_create(&tid, NULL, thread_client, arg);
      pthread_detach(tid);
    }
  }
}

void *thread_client(void *args)
{
  int connfd = *((int *)args);
  free(args);
  doit(connfd);
  return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);

  if (!parse_request_line(buf, &method, &uri, &version))
  {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  }
  else
  {
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1"))
    {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    }
    else if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    }
    else
    {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      print_stringdictionary(query);

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */

      if (starts_with("/friends", uri))
      {
        getFriends(fd, query);
      }
      else if (starts_with("/befriend", uri))
      {
        beFriends(fd, query);
      }
      else if (starts_with("/unfriend", uri))
      {
        unFriend(fd, query);
      }
      else if (starts_with("/introduce", uri))
      {
        introduceFriend(fd, query);
      }
      else
      {
        serve_request(fd, query);
      }

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);

    close(fd);
  }
}

static void getFriends(int fd, dictionary_t *query)
{
  char *user = dictionary_get(query, "user");

  pthread_mutex_lock(&mutex);
  dictionary_t *user_Friends_Dictionary = registerClient(AllClients, user);
  pthread_mutex_unlock(&mutex);

  if (user_Friends_Dictionary == NULL)
  {
    return;
  }

  char *body = getBody(user_Friends_Dictionary);

  WriteResponse(fd, body);

  free(body);
}

static void beFriends(int fd, dictionary_t *query)
{
  const char *user;
  char *friends;
  user = dictionary_get(query, "user");
  friends = dictionary_get(query, "friends");

  pthread_mutex_lock(&mutex);
  char *body = UpdateUserFriends(friends, user);
  pthread_mutex_unlock(&mutex);
  WriteResponse(fd, body);

  free(body);
}

static void introduceFriend(int fd, dictionary_t *query)
{
  // Get info from query
  const char *user;
  char *friends;
  user = dictionary_get(query, "user");
  friends = dictionary_get(query, "friend");
  char *host = dictionary_get(query, "host");
  char *port = dictionary_get(query, "port");
  char **friends_array = split_string(friends, '\n');

  // if the user is not registered in the dictionary
  pthread_mutex_lock(&mutex);
  dictionary_t *user_Friends_Dictionary = registerClient(AllClients, user);
  pthread_mutex_unlock(&mutex);
  int count = 0;
  while (friends_array[count] != NULL)
  {
    char *FriFriends = getFriFriend(host, port, friends_array[count]);
    printf("Debug FriFriends at introduce from getFriFriend: %s\n", FriFriends);
    pthread_mutex_lock(&mutex);
    UpdateUserFriends(FriFriends, user);
    pthread_mutex_unlock(&mutex);

    count++;
  }

  char *body = getBody(user_Friends_Dictionary);

  WriteResponse(fd, body);
}

static void unFriend(int fd, dictionary_t *query)
{
  const char *user;
  char *friends;
  user = dictionary_get(query, "user");
  friends = dictionary_get(query, "friends");
  char **friends_array = split_string(friends, '\n');

  pthread_mutex_lock(&mutex);
  dictionary_t *user_Friends_Dictionary = registerClient(AllClients, user);
  int count = 0;
  while (friends_array[count] != NULL)
  {
    const char *current_Friend = friends_array[count];
    for (int i = 0; i < dictionary_count(user_Friends_Dictionary); i++)
    {
      if (strcmp(dictionary_key(user_Friends_Dictionary, i), current_Friend) == 0)
      {
        dictionary_remove(user_Friends_Dictionary, current_Friend);

        dictionary_t *friend_Friends = dictionary_get(AllClients, current_Friend);
        dictionary_remove(friend_Friends, user);
      }
    }

    count++;
  }
  pthread_mutex_unlock(&mutex);

  char *body = getBody(user_Friends_Dictionary);

  WriteResponse(fd, body);

  free(body);
}

static dictionary_t *registerClient(dictionary_t *query, const char *user)
{
  if (dictionary_get(query, user) == NULL)
  {
    dictionary_set(query, user, make_dictionary(COMPARE_CASE_SENS, free));
  }

  dictionary_t *user_Friends_Dictionary = dictionary_get(AllClients, user);
  return user_Friends_Dictionary;
}

static void WriteResponse(int fd, char *body)
{
  size_t len = strlen(body);
  /* Send response headers to client */
  char *header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);
}

static char *UpdateUserFriends(char *newFriends, const char *user)
{
  dictionary_t *user_Friends_Dictionary = registerClient(AllClients, user);
  char **friends_array = split_string(newFriends, '\n');

  int count = 0;
  while (friends_array[count] != NULL)
  {
    if (strcmp(friends_array[count], user) == 0)
    {
      count++;
      continue;
    }
    const char *current_Friend = friends_array[count];
    dictionary_set(user_Friends_Dictionary, current_Friend, NULL);

    // if the friend is not registered in the dictionary
    registerClient(AllClients, current_Friend);

    dictionary_t *friend_Friends = dictionary_get(AllClients, current_Friend);
    dictionary_set(friend_Friends, user, NULL);

    count++;
  }

  char *body = getBody(user_Friends_Dictionary);

  return body;
}

static char *getFriFriend(char *host, char *port, char *friends)
{
  int connection_fd = open_clientfd(host, port);
  pthread_mutex_lock(&mutex);
  char *friend_encode = query_encode(friends);
  char http_request[MAXLINE];
  sprintf(http_request, "GET /friends?user=%s HTTP/1.0\r\n\r\n", friend_encode); // makes http_request = GET /friends?user=alice HTTP/1.0\r\n\r\n
  rio_writen(connection_fd, http_request, strlen(http_request));
  pthread_mutex_unlock(&mutex);

  char buf[MAXLINE]; // Buffer to store the response
  rio_t rio;
  Rio_readinitb(&rio, connection_fd);
  Rio_readlineb(&rio, buf, MAXLINE); // Read the first line of the response

  dictionary_t *response = read_requesthdrs(&rio);
  parse_header_line(buf, response);
  
  char *content_length_str = dictionary_get(response, "Content-Length");
  int content_length = atoi(content_length_str);
  
  // pthread_mutex_lock(&mutex);
  char *content = malloc(content_length + 1); // +1 for the null terminator
  ssize_t num_bytes = rio_readn(connection_fd, content, content_length);
  if (num_bytes > 0)
  {
    content[num_bytes] = '\0'; // Null-terminate the content
  }
  // pthread_mutex_unlock(&mutex);

  printf("Debug Content: %s\n", content);
  close(connection_fd);
  return content;
}

static char *getBody(dictionary_t *user_Friends_Dictionary)
{
  char *body = join_strings(dictionary_keys(user_Friends_Dictionary), '\n');
  return body;
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }

  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;

  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");

  buffer = malloc(len + 1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded"))
  {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type)
{
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_request - example request handler
 */
static void serve_request(int fd, dictionary_t *query)
{
  size_t len;
  char *body, *header;

  body = strdup("alice\nbob");

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor="
                        "ffffff"
                        ">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);

  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++)
  {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
