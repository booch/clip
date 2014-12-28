/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_HTTPSERVICE_H
#define _FNORDMETRIC_HTTPSERVICE_H
#include <fnord/net/http/httphandler.h>
#include <fnord/net/http/httprequest.h>
#include <fnord/net/http/httpresponse.h>
#include "fnord/thread/taskscheduler.h"

namespace fnord {
namespace http {

class HTTPService {
public:

  virtual void handleHTTPRequest(
      HTTPRequest* req,
      HTTPResponse* res) = 0;

};

class HTTPServiceHandler : public HTTPHandler {
public:
  HTTPServiceHandler(
      HTTPService* service,
      thread::TaskScheduler* scheduler,
      HTTPServerConnection* conn,
      HTTPRequest* req);

  void handleHTTPRequest() override;
protected:
  void dispatchRequest();

  HTTPService* service_;
  thread::TaskScheduler* scheduler_;
  HTTPServerConnection* conn_;
  HTTPRequest* req_;
  HTTPResponse res_;
};

}
}
#endif
