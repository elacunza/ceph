// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2021 Red Hat
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#include <errno.h>

#include <iostream>
#include <string>

#include <fmt/format.h>

#include "test/client/TestClient.h"

#define dout_subsys ceph_subsys_client

class C_SaferCond_t : public C_SaferCond {
public:
  C_SaferCond_t(const std::string &name) :
    C_SaferCond(name)
  {}
  void complete(int r) override {
  	ldout(g_ceph_context, 0) << "complete " << this << " << rc " << r << dendl;
  	C_SaferCond::complete(r);
  }
};

#if 1
TEST_F(TestClient, LlreadvLlwritev) {
  int mypid = getpid();
  char filename[256];

  client->unmount();
  TearDown();
  SetUp();

  sprintf(filename, "test_llreadvllwritevfile%u", mypid);

  Inode *root, *file;
  root = client->get_root();
  ASSERT_NE(root, (Inode *)NULL);

  Fh *fh;
  struct ceph_statx stx;

  ASSERT_EQ(0, client->ll_createx(root, filename, 0666,
				  O_RDWR | O_CREAT | O_TRUNC,
				  &file, &fh, &stx, 0, 0, myperm));

  /* Reopen read-only */
  char out0[] = "hello ";
  char out1[] = "world\n";
  struct iovec iov_out[2] = {
	{out0, sizeof(out0)},
	{out1, sizeof(out1)},
  };
  char in0[sizeof(out0)];
  char in1[sizeof(out1)];
  struct iovec iov_in[2] = {
	{in0, sizeof(in0)},
	{in1, sizeof(in1)},
  };

  ssize_t nwritten = iov_out[0].iov_len + iov_out[1].iov_len;
  int64_t rc;
  bufferlist bl;

#if 1
  std::unique_ptr<C_SaferCond> writefinish = nullptr;
  std::unique_ptr<C_SaferCond> readfinish = nullptr;

  writefinish.reset(new C_SaferCond_t("test-async"));
  readfinish.reset(new C_SaferCond_t("test-async"));
  rc = client->ll_preadv_pwritev(fh, iov_out, 2, 0, true, writefinish.get(), nullptr);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "ll_preadv_pwritev for write returned " << rc << dendl;
  ASSERT_EQ(0, rc);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "wait_for for write " << writefinish.get() << dendl;
  rc = writefinish.get()->wait_for(100);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "wait_for for write returned " << rc << dendl;
  ASSERT_EQ(nwritten, rc);

  rc = client->ll_preadv_pwritev(fh, iov_in, 2, 0, false, readfinish.get(), &bl);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "ll_preadv_pwritev for read returned " << rc << dendl;
  ASSERT_EQ(0, rc);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "wait_for for read " << readfinish.get() << dendl;
  rc = readfinish.get()->wait_for(100);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "wait_for for read returned " << rc << dendl;
  ASSERT_EQ(nwritten, rc);
#else
  rc = client->ll_preadv_pwritev_t(fh, iov_out, 2, 0, true, nullptr);
  ASSERT_EQ(nwritten, rc);

  rc = client->ll_preadv_pwritev_t(fh, iov_in, 2, 0, false, &bl);
  ASSERT_EQ(nwritten, rc);
#endif
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "calling copy_bufferlist_to_iovec"
                           << " bl.length() = " << bl.length()
                           << " &bl " << &bl
                           << dendl;
  copy_bufferlist_to_iovec(iov_in, 2, &bl, rc);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "done copy_bufferlist_to_iovec" << dendl;
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;

  rc = strncmp((const char*)iov_in[0].iov_base, (const char*)iov_out[0].iov_base, iov_out[0].iov_len);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "strncmp first segment " << rc << dendl;
  ASSERT_EQ(0, rc);
  rc = strncmp((const char*)iov_in[1].iov_base, (const char*)iov_out[1].iov_base, iov_out[1].iov_len);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "strncmp second segment " << rc << dendl;
  ASSERT_EQ(0, rc);

  client->ll_release(fh);
  rc = client->ll_unlink(root, filename, myperm);
  ldout(g_ceph_context, 0) << "==================================================================================================================== " << dendl;
  ldout(g_ceph_context, 0) << "ll_unlink returned " << rc << dendl;
  ASSERT_EQ(0, rc);
}
#endif

