/* CFNotificationCenter.m

   Copyright (C) 2025 Free Software Foundation, Inc.

   Written by: John Googler
   Date: July, 2025

   This file is part of GNUstep CoreBase Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; see the file COPYING.LIB.
   If not, see <http://www.gnu.org/licenses/> or write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#import <Foundation/NSNotification.h>
#import <Foundation/NSDistributedNotificationCenter.h>
#import <Foundation/NSString.h>
#import <Foundation/NSDictionary.h>
#import <Foundation/NSArray.h>

#include "CoreFoundation/CFRuntime.h"
#include "CoreFoundation/CFBase.h"
#include "CoreFoundation/CFNotificationCenter.h"
#include "CoreFoundation/CFDictionary.h"
#include "CoreFoundation/CFString.h"
#include "CoreFoundation/CFArray.h"

#include "GSPrivate.h"
#include "GSObjCRuntime.h"
#import <Foundation/NSDictionary.h>

/* A record representing a single observer registration. */
typedef struct __CFNotifObserverRecord
{
  const void               *observer;
  CFNotificationCallback    callBack;
  CFStringRef               name;       /* retained, may be NULL */
  const void               *object;     /* NOT retained */
  id                        nsObserver; /* the opaque token from NSNotificationCenter */
} _CFNotifObserverRecord;

struct __CFNotificationCenter
{
  CFRuntimeBase       _parent;
  id                  _nsCenter;       /* NSNotificationCenter (not retained, singleton) */
  GSMutex             _lock;
  _CFNotifObserverRecord *_records;
  CFIndex             _recordCount;
  CFIndex             _recordCapacity;
};

static CFTypeID _kCFNotificationCenterTypeID = 0;
static CFNotificationCenterRef _localCenter = NULL;
static CFNotificationCenterRef _distributedCenter = NULL;

static void
CFNotificationCenterFinalize (CFTypeRef cf)
{
  CFNotificationCenterRef center = (CFNotificationCenterRef)cf;
  CFIndex i;

  for (i = 0; i < center->_recordCount; i++)
    {
      _CFNotifObserverRecord *rec = &center->_records[i];
      if (rec->nsObserver)
        [(NSNotificationCenter *)center->_nsCenter removeObserver:rec->nsObserver];
      if (rec->name)
        CFRelease (rec->name);
    }

  if (center->_records)
    CFAllocatorDeallocate (kCFAllocatorDefault, center->_records);

  GSMutexDestroy (&center->_lock);
}

static const CFRuntimeClass CFNotificationCenterClass =
{
  0,
  "CFNotificationCenter",
  NULL,
  NULL,
  CFNotificationCenterFinalize,
  NULL,
  NULL,
  NULL,
  NULL
};

void CFNotificationCenterInitialize (void)
{
  _kCFNotificationCenterTypeID =
    _CFRuntimeRegisterClass (&CFNotificationCenterClass);
}

CFTypeID
CFNotificationCenterGetTypeID (void)
{
  return _kCFNotificationCenterTypeID;
}

#define CFNOTIFICATIONCENTER_SIZE \
  sizeof(struct __CFNotificationCenter) - sizeof(CFRuntimeBase)

static CFNotificationCenterRef
CFNotificationCenterCreate (id nsCenter)
{
  struct __CFNotificationCenter *new;

  new = (struct __CFNotificationCenter *)_CFRuntimeCreateInstance (
    kCFAllocatorDefault,
    _kCFNotificationCenterTypeID,
    CFNOTIFICATIONCENTER_SIZE,
    0);

  if (new)
    {
      new->_nsCenter = nsCenter;
      GSMutexInitialize (&new->_lock);
      new->_records = NULL;
      new->_recordCount = 0;
      new->_recordCapacity = 0;
    }

  return (CFNotificationCenterRef)new;
}

CFNotificationCenterRef
CFNotificationCenterGetLocalCenter (void)
{
  if (_localCenter == NULL)
    {
      CFNotificationCenterRef center =
        CFNotificationCenterCreate ([NSNotificationCenter defaultCenter]);
      if (GSAtomicCompareAndSwapPointer ((void **)&_localCenter, NULL, center)
          != NULL)
        CFRelease (center);
    }
  return _localCenter;
}

CFNotificationCenterRef
CFNotificationCenterGetDistributedCenter (void)
{
  if (_distributedCenter == NULL)
    {
      CFNotificationCenterRef center =
        CFNotificationCenterCreate (
          [NSDistributedNotificationCenter defaultCenter]);
      if (GSAtomicCompareAndSwapPointer ((void **)&_distributedCenter, NULL,
                                         center) != NULL)
        CFRelease (center);
    }
  return _distributedCenter;
}

static void
_CFNotificationCenterGrowRecords (CFNotificationCenterRef center)
{
  if (center->_recordCount >= center->_recordCapacity)
    {
      CFIndex newCap = center->_recordCapacity == 0
        ? 8 : center->_recordCapacity * 2;
      _CFNotifObserverRecord *newBuf;

      newBuf = (_CFNotifObserverRecord *)CFAllocatorReallocate (
        kCFAllocatorDefault, center->_records,
        newCap * sizeof (_CFNotifObserverRecord), 0);

      center->_records = newBuf;
      center->_recordCapacity = newCap;
    }
}

void
CFNotificationCenterAddObserver (CFNotificationCenterRef center,
  const void *observer, CFNotificationCallback callBack, CFStringRef name,
  const void *object, CFNotificationSuspensionBehavior suspensionBehavior)
{
  _CFNotifObserverRecord rec;
  NSString *nsName = nil;
  id nsObject = nil;
  id nsObserverToken;

  if (center == NULL || callBack == NULL)
    return;

  if (name != NULL)
    nsName = (NSString *)name;
  if (object != NULL)
    nsObject = (id)object;

  /* Capture the values we need for the block. */
  const void *capturedObserver = observer;
  CFNotificationCallback capturedCallback = callBack;
  CFNotificationCenterRef capturedCenter = center;

  nsObserverToken = [(NSNotificationCenter *)center->_nsCenter
    addObserverForName:nsName
                object:nsObject
                 queue:nil
            usingBlock:^(NSNotification *note) {
      CFStringRef cfName = (CFStringRef)[note name];
      const void *cfObject = (const void *)[note object];
      CFDictionaryRef cfUserInfo = (CFDictionaryRef)[note userInfo];
      capturedCallback (capturedCenter, (void *)capturedObserver,
                        cfName, cfObject, cfUserInfo);
    }];

  rec.observer = observer;
  rec.callBack = callBack;
  rec.name = name ? (CFStringRef)CFRetain (name) : NULL;
  rec.object = object;
  rec.nsObserver = [nsObserverToken retain];

  GSMutexLock (&center->_lock);
  _CFNotificationCenterGrowRecords (center);
  center->_records[center->_recordCount++] = rec;
  GSMutexUnlock (&center->_lock);
}

void
CFNotificationCenterRemoveObserver (CFNotificationCenterRef center,
  const void *observer, CFNotificationName name, const void *object)
{
  CFIndex i;

  if (center == NULL)
    return;

  GSMutexLock (&center->_lock);
  for (i = center->_recordCount - 1; i >= 0; i--)
    {
      _CFNotifObserverRecord *rec = &center->_records[i];
      if (rec->observer != observer)
        continue;
      if (name != NULL && (rec->name == NULL || !CFEqual (rec->name, name)))
        continue;
      if (object != NULL && rec->object != object)
        continue;

      /* Remove from NSNotificationCenter */
      [(NSNotificationCenter *)center->_nsCenter
        removeObserver:rec->nsObserver];
      [rec->nsObserver release];
      if (rec->name)
        CFRelease (rec->name);

      /* Compact the array */
      if (i < center->_recordCount - 1)
        center->_records[i] = center->_records[center->_recordCount - 1];
      center->_recordCount--;
    }
  GSMutexUnlock (&center->_lock);
}

void
CFNotificationCenterRemoveEveryObserver (CFNotificationCenterRef center,
  const void *observer)
{
  CFIndex i;

  if (center == NULL)
    return;

  GSMutexLock (&center->_lock);
  for (i = center->_recordCount - 1; i >= 0; i--)
    {
      _CFNotifObserverRecord *rec = &center->_records[i];
      if (rec->observer != observer)
        continue;

      [(NSNotificationCenter *)center->_nsCenter
        removeObserver:rec->nsObserver];
      [rec->nsObserver release];
      if (rec->name)
        CFRelease (rec->name);

      if (i < center->_recordCount - 1)
        center->_records[i] = center->_records[center->_recordCount - 1];
      center->_recordCount--;
    }
  GSMutexUnlock (&center->_lock);
}

void
CFNotificationCenterPostNotification (CFNotificationCenterRef center,
  CFNotificationName name, const void *object, CFDictionaryRef userInfo,
  Boolean deliverImmediately)
{
  CFOptionFlags options = 0;

  if (deliverImmediately)
    options |= kCFNotificationDeliverImmediately;

  CFNotificationCenterPostNotificationWithOptions (center, name, object,
    userInfo, options);
}

void
CFNotificationCenterPostNotificationWithOptions (CFNotificationCenterRef center,
  CFNotificationName name, const void *object, CFDictionaryRef userInfo,
  CFOptionFlags options)
{
  NSString *nsName;
  id nsObject;
  NSDictionary *nsUserInfo;

  if (center == NULL || name == NULL)
    return;

  nsName = (NSString *)name;
  nsObject = (id)object;
  nsUserInfo = (NSDictionary *)userInfo;

  [(NSNotificationCenter *)center->_nsCenter
    postNotificationName:nsName
                  object:nsObject
                userInfo:nsUserInfo];
}
