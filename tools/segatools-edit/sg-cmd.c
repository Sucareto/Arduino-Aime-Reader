#include <assert.h>

#include "board/sg-cmd.h"
#include "board/sg-frame.h"

#include "hook/iobuf.h"

#include "util/dprintf.h"

union sg_req_any {
    struct sg_req_header req;
    uint8_t bytes[256];
};

union sg_res_any {
    struct sg_res_header res;
    uint8_t bytes[256];
};

static HRESULT sg_req_validate(const void *ptr, size_t nbytes);

static void sg_res_error(
        struct sg_res_header *res,
        const struct sg_req_header *req);

static HRESULT sg_req_validate(const void *ptr, size_t nbytes)
{
    const struct sg_req_header *req;
    size_t payload_len;

    assert(ptr != NULL);

    if (nbytes < sizeof(*req)) {
        dprintf("SG Cmd: Request header truncated\n");

        return E_FAIL;
    }

    req = ptr;

    if (req->hdr.frame_len != nbytes) {
        dprintf("SG Cmd: Frame length mismatch: got %i exp %i\n",
                req->hdr.frame_len,
                (int) nbytes);

        return E_FAIL;
    }

    payload_len = req->hdr.frame_len - sizeof(*req);

    if (req->payload_len != payload_len) {
        dprintf("SG Cmd: Payload length mismatch: got %i exp %i\n",
                req->payload_len,
                (int) payload_len);

        return E_FAIL;
    }

    return S_OK;
}

void sg_req_transact(
        struct iobuf *res_frame,
        const uint8_t *req_bytes,
        size_t req_nbytes,
        sg_dispatch_fn_t dispatch,
        void *ctx)
{
    struct iobuf req_span;
    union sg_req_any req;
    union sg_res_any res;
    HRESULT hr;

    assert(res_frame != NULL);
    assert(req_bytes != NULL);
    assert(dispatch != NULL);

    req_span.bytes = req.bytes;
    req_span.nbytes = sizeof(req.bytes);
    req_span.pos = 0;

    hr = sg_frame_decode(&req_span, req_bytes, req_nbytes);

    if (FAILED(hr)) {
        return;
    }

    hr = sg_req_validate(req.bytes, req_span.pos);

    if (FAILED(hr)) {
        return;
    }

    hr = dispatch(ctx, &req, &res);

    if (hr != S_FALSE) {
        if (FAILED(hr)) {
            sg_res_error(&res.res, &req.req);
        }

        sg_frame_encode(res_frame, res.bytes, res.res.hdr.frame_len);
        printf("req: ");
        for (uint8_t i = 0; i < req_nbytes; i++)
            printf("%02X ", req_bytes[i]);
        printf("\n");
        printf("res: ");
        for (uint8_t i = 0; i < res_frame->pos; i++)
            printf("%02X ", res_frame->bytes[i]);
        printf("\n");
    }
}

void sg_res_init(
        struct sg_res_header *res,
        const struct sg_req_header *req,
        size_t payload_len)
{
    assert(res != NULL);
    assert(req != NULL);

    res->hdr.frame_len = sizeof(*res) + payload_len;
    res->hdr.addr = req->hdr.addr;
    res->hdr.seq_no = req->hdr.seq_no;
    res->hdr.cmd = req->hdr.cmd;
    res->status = 0;
    res->payload_len = payload_len;
}

static void sg_res_error(
        struct sg_res_header *res,
        const struct sg_req_header *req)
{
    assert(res != NULL);
    assert(req != NULL);

    res->hdr.frame_len = sizeof(*res);
    res->hdr.addr = req->hdr.addr;
    res->hdr.seq_no = req->hdr.seq_no;
    res->hdr.cmd = req->hdr.cmd;
    res->status = 1;
    res->payload_len = 0;
}
