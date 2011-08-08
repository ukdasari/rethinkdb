#include "riak/proto_server.hpp"
#include "riak/proto/riakclient.pb.h"
#include <google/protobuf/message.h>
#include "riak/riak_interface.hpp"


namespace riak {


void proto_server_t::handle_conn(boost::scoped_ptr<tcp_conn_t> &conn) {
    for (;;) {
        message_size_t size;
        message_code_t mc;

        conn->read(&size, sizeof(size));
        conn->read(&mc, sizeof(mc));

        const_charslice slc = conn->peek(size);

        switch (mc) {
        case RpbPingReq:
            handle<dummy_msgs::RpbPingReq>(slc, conn);
            break;
        case RpbGetClientIdReq:
            handle<dummy_msgs::RpbGetClientIdReq>(slc, conn);
            break;
        case RpbSetClientIdReq:
            handle< ::RpbSetClientIdReq>(slc, conn);
            break;
        case RpbGetServerInfoReq:
            handle<dummy_msgs::RpbGetServerInfoReq>(slc, conn);
            break;
        case RpbGetReq:
            handle< ::RpbGetReq>(slc, conn);
            break;
        case RpbPutReq:
            handle< ::RpbPutReq>(slc, conn);
            break;
        case RpbDelReq:
            handle< ::RpbDelReq>(slc, conn);
            break;
        case RpbListBucketsReq:
            handle<dummy_msgs::RpbListBucketsReq>(slc, conn);
            break;
        case RpbListKeysReq:
            handle< ::RpbListKeysReq>(slc, conn);
            break;
        case RpbGetBucketReq:
            handle< ::RpbGetBucketReq>(slc, conn);
            break;
        case RpbSetBucketReq:
            handle< ::RpbSetBucketReq>(slc, conn);
            break;
        case RpbMapRedReq:
            handle< ::RpbMapRedReq>(slc, conn);
            break;
        default:
            unreachable();
        }

        conn->pop(size);
    }
}

proto_server_t::proto_server_t(int port, riak_interface_t *riak_interface)
    : riak_interface(riak_interface)
{ 
    tcp_listener.reset(new tcp_listener_t(port, boost::bind(&proto_server_t::handle_conn, this, _1)));
}

proto_server_t::~proto_server_t() { }

template <class T>
void proto_server_t::handle(const_charslice &slc, boost::scoped_ptr<tcp_conn_t> &conn) {
    T msg;
    msg.ParseFromArray(slc.beg, slc.end - slc.beg);
    handle_msg(msg, conn);
}


void proto_server_t::handle_msg(dummy_msgs::RpbPingReq &, boost::scoped_ptr<tcp_conn_t> &conn) {
    message_size_t size = 0;
    message_code_t mc = RpbPingResp;
    conn->write(&size, sizeof(message_size_t));
    conn->write(&mc, sizeof(message_code_t));
}
void proto_server_t::handle_msg(dummy_msgs::RpbGetClientIdReq &, boost::scoped_ptr<tcp_conn_t> &conn) {
    ::RpbGetClientIdResp res;
    res.set_client_id("None");
    write_to_conn(res, conn);
}
void proto_server_t::handle_msg(::RpbSetClientIdReq &, boost::scoped_ptr<tcp_conn_t> &conn) {
    //TODO, actually set some clientID state and such
    message_size_t size = 0;
    message_code_t mc = RpbSetClientIdResp;
    conn->write(&size, sizeof(message_size_t));
    conn->write(&mc, sizeof(message_code_t));
}

void proto_server_t::handle_msg(dummy_msgs::RpbGetServerInfoReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(::RpbGetReq &msg, boost::scoped_ptr<tcp_conn_t> &conn) {
    ::RpbGetResp res;
    ::RpbContent *content = res.add_content();

    object_t obj = riak_interface->get_object(msg.bucket(), msg.key());
    content->set_value(obj.content.get(), obj.content_length);
    content->set_content_type(obj.content_type);

    for (std::vector<link_t>::const_iterator it = obj.links.begin(); it != obj.links.end(); it++) {
        RpbLink* link = content->add_links();
        link->set_bucket(it->bucket);
        link->set_key(it->key);
        link->set_tag(it->tag);
    }

    content->set_last_mod(obj.last_written);

    write_to_conn(res, conn);
}

void proto_server_t::handle_msg(::RpbPutReq &msg, boost::scoped_ptr<tcp_conn_t> &conn) {
    object_t obj;

    obj.key = msg.key();
    RpbContent content = msg.content();
    obj.content_type = content.content_type();
    obj.set_content(content.value().data(), content.value().size());

    for (int i = 0; i < content.links_size(); i++) {
        link_t l = { content.links(i).bucket(), content.links(i).key(), content.links(i).tag() };
        obj.links.push_back(l);
    }

    riak_interface->store_object(msg.bucket(), obj);

    ::RpbPutResp res;
    if (msg.return_body()) {
        res.add_content()->CopyFrom(msg.content());
        //TODO maybe we should set the last modified time here ???
    }

    write_to_conn(res, conn);
}

void proto_server_t::handle_msg(::RpbDelReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(dummy_msgs::RpbListBucketsReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(::RpbListKeysReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(::RpbGetBucketReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(::RpbSetBucketReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

void proto_server_t::handle_msg(::RpbMapRedReq &, boost::scoped_ptr<tcp_conn_t> &) {
}

} //namespace riak
