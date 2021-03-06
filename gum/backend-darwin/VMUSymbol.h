/*
 *     Generated by class-dump 3.3.3 (64 bit).
 *
 *     class-dump is Copyright (C) 1997-1998, 2000-2001, 2004-2010 by Steve Nygard.
 */

#import "VMUAddressRange.h"
#import "VMUSourceInfo.h"
#import "VMUSymbolOwner.h"

@class VMUSymbolOwner;

@interface VMUSymbol : VMUAddressRange <NSCopying>
{
    NSString *_name;
    NSString *_mangledName;
    VMUSymbolOwner *_owner;
    unsigned int _flags;
}

+ (id)symbolWithName:(id)arg1 mangledName:(id)arg2 addressRange:(struct _VMURange)arg3 owner:(VMUSymbolOwner *)arg4 flags:(unsigned int)arg5;
- (id)initWithName:(id)arg1 mangledName:(id)arg2 addressRange:(struct _VMURange)arg3 owner:(VMUSymbolOwner *)arg4 flags:(unsigned int)arg5;
- (NSString *)name;
- (NSString *)mangledName;
- (struct _VMURange)addressRange;
- (VMUSymbolOwner *)owner;
- (id)sourceInfos;
- (id)sourceInfoForAddress:(unsigned long long)arg1;
- (id)sourceInfosInAddressRange:(struct _VMURange)arg1;
- (id)text;
- (unsigned int)flags;
- (BOOL)isFunction;
- (BOOL)isObjcMethod;
- (BOOL)isJavaMethod;
- (BOOL)isDyldStub;
- (BOOL)isExternal;
- (BOOL)isStab;
- (BOOL)isDwarf;
- (long long)compare:(id)arg1;
- (BOOL)isEqualToSymbol:(id)arg1;
- (NSString *)description;
- (void)dealloc;
- (id)copyWithZone:(struct _NSZone *)arg1;

@end

