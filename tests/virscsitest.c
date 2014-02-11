/*
 * Copyright (C) 2014 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Author: Osier Yang <jyang@redhat.com>
 *
 */

#include <config.h>

#include <stdlib.h>

#include "virscsi.h"
#include "testutils.h"

#define VIR_FROM_THIS VIR_FROM_NONE

#define VIR_SCSI_DATA "/virscsidata"

static const char *abs_top_srcdir;
static char *virscsi_prefix = NULL;

static int
test1(const void *data ATTRIBUTE_UNUSED)
{
    char *name = NULL;
    int ret = -1;

    if (!(name = virSCSIDeviceGetDevName(virscsi_prefix,
                                         "scsi_host1", 0, 0, 0)))
        return -1;

    if (STRNEQ(name, "sdh"))
        goto cleanup;

    ret = 0;
cleanup:
    VIR_FREE(name);
    return ret;
}

/*
 * Two test devices are used, one has address "0:0:0:0", the
 * other has address "1:0:0:0", see "virscsidata/" for more
 * details.
 */
static int
test2(const void *data ATTRIBUTE_UNUSED)
{
    virSCSIDeviceListPtr list = NULL;
    virSCSIDevicePtr dev = NULL;
    virSCSIDevicePtr dev1 = NULL;
    bool free_dev = true;
    bool free_dev1 = true;
    virSCSIDevicePtr tmp = NULL;
    char *sgname = NULL;
    int ret = -1;

    sgname = virSCSIDeviceGetSgName(virscsi_prefix,
                                    "scsi_host1", 0, 0, 0);

    if (!sgname || STRNEQ(sgname, "sg8"))
        goto cleanup;

    if (!(dev = virSCSIDeviceNew(virscsi_prefix, "scsi_host1",
                                 0, 0, 0, false, true)))
        goto cleanup;

    if (STRNEQ_NULLABLE(virSCSIDeviceGetName(dev), "1:0:0:0") ||
        virSCSIDeviceGetAdapter(dev) != 1 ||
        virSCSIDeviceGetBus(dev) != 0 ||
        virSCSIDeviceGetTarget(dev) != 0 ||
        virSCSIDeviceGetUnit(dev) != 0 ||
        virSCSIDeviceGetReadonly(dev) ||
        !virSCSIDeviceGetShareable(dev))
        goto cleanup;

    if (!virSCSIDeviceIsAvailable(dev))
        goto cleanup;

    if (virSCSIDeviceSetUsedBy(dev, "fc18") < 0)
        goto cleanup;

    if (virSCSIDeviceIsAvailable(dev))
        goto cleanup;

    if (virSCSIDeviceSetUsedBy(dev, "fc20") < 0)
        goto cleanup;

    if (virSCSIDeviceIsAvailable(dev))
        goto cleanup;

    if (!(list = virSCSIDeviceListNew()))
        goto cleanup;

    if (virSCSIDeviceListAdd(list, dev) < 0)
        goto cleanup;

    /* virSCSIDeviceListDispose will take care of freeing
     * the device.
     */
    free_dev = false;

    if (!virSCSIDeviceListFind(list, dev))
        goto cleanup;

    virSCSIDeviceListDel(list, dev, "fc20");

    if (!virSCSIDeviceListFind(list, dev))
        goto cleanup;

    if (virSCSIDeviceIsAvailable(dev))
        goto cleanup;

    if (virSCSIDeviceListCount(list) != 1)
        goto cleanup;

    if (!(dev1 = virSCSIDeviceNew(virscsi_prefix, "scsi_host0",
                                  0, 0, 0, true, false)))
        goto cleanup;

    if (virSCSIDeviceListAdd(list, dev1) < 0)
        goto cleanup;

    /* virSCSIDeviceListDispose will take care of freeing
     * the device.
     */
    free_dev1 = false;

    if (virSCSIDeviceListCount(list) != 2)
        goto cleanup;

    if (!(tmp = virSCSIDeviceListSteal(list, dev1)))
        goto cleanup;
    virSCSIDeviceFree(tmp);

    if (virSCSIDeviceListCount(list) != 1)
        goto cleanup;

    ret = 0;
cleanup:
    VIR_FREE(sgname);
    if (free_dev)
        virSCSIDeviceFree(dev);
    if (free_dev1)
        virSCSIDeviceFree(dev1);
    virObjectUnref(list);
    return ret;
}

static int
mymain(void)
{
    int ret = 0;

    abs_top_srcdir = getenv("abs_top_srcdir");
    if (!abs_top_srcdir)
        abs_top_srcdir = abs_srcdir "/..";

    if (virAsprintf(&virscsi_prefix, "%s" VIR_SCSI_DATA, abs_srcdir) < 0) {
        ret = -1;
        goto cleanup;
    }

    if (virtTestRun("test1", test1, NULL) < 0)
        ret = -1;
    if (virtTestRun("test2", test2, NULL) < 0)
        ret = -1;

cleanup:
    VIR_FREE(virscsi_prefix);
    return ret;
}

VIRT_TEST_MAIN(mymain)