#import <Foundation/Foundation.h>
// Rewrite only visual intensity fields. Preserve file length and every gameplay field.
NSData *PGRewriteVisualEffects(NSData *data,BOOL sun,BOOL unused){
 NSString *text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];if(!text)return nil;
 NSMutableString *out=[text mutableCopy];BOOL changed=NO;
 NSArray *fields=@[@"light_ray_mult",@"Lensflare_visibility"];
 for(NSString *field in fields){
  if(!sun)continue;
  NSRegularExpression *re=[NSRegularExpression regularExpressionWithPattern:[NSString stringWithFormat:@"(<%@>)([^<]*)(</%@>)",field,field] options:NSRegularExpressionCaseInsensitive error:nil];
  for(NSTextCheckingResult *m in [re matchesInString:out options:0 range:NSMakeRange(0,out.length)].reverseObjectEnumerator){
   NSRange range=[m rangeAtIndex:2];NSMutableString *values=[[out substringWithRange:range] mutableCopy];
   NSRegularExpression *numbers=[NSRegularExpression regularExpressionWithPattern:@"[-+]?[0-9]+(?:\\.[0-9]+)?(?:[eE][-+]?[0-9]+)?" options:0 error:nil];
   for(NSTextCheckingResult *n in [numbers matchesInString:values options:0 range:NSMakeRange(0,values.length)].reverseObjectEnumerator){
    NSMutableString *zero=[NSMutableString stringWithString:@"0"];while(zero.length<n.range.length)[zero appendString:@" "];[values replaceCharactersInRange:n.range withString:zero];
   }
   [out replaceCharactersInRange:range withString:values];changed=YES;
  }
 }
 NSData *result=[out dataUsingEncoding:NSUTF8StringEncoding];return changed&&result.length==data.length?result:nil;
}

NSData *PGRewriteExplosionEffects(NSData *data){
 NSString *text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];if(![text hasPrefix:@"4.00"])return nil;
 NSSet *tags=[NSSet setWithArray:@[@"EXP_VFXTAG_GRENADE",@"EXP_VFXTAG_STICKYBOMB",@"EXP_VFXTAG_ROCKET",@"EXP_VFXTAG_VEHICLE",@"EXP_VFXTAG_BIKE",@"EXP_VFXTAG_TRUCK",@"EXP_VFXTAG_BULLET",@"EXP_VFXTAG_PETROL_PUMP",@"EXP_VFXTAG_PLANE_ROCKET",@"EXP_VFXTAG_TANKER",@"EXP_VFXTAG_PLANE",@"EXP_VFXTAG_BOAT",@"EXP_VFXTAG_TANKSHELL",@"EXP_VFXTAG_BARREL",@"EXP_VFXTAG_PROPANE",@"EXP_VFXTAG_TRAIN",@"EXP_VFXTAG_FBI4_TRUCK_DOORS",@"EXP_VFXTAG_TREV3_TRAILER",@"EXP_VFXTAG_BLIMP",@"EXP_VFXTAG_BLIMP2"]];
 NSMutableString *out=[text mutableCopy];BOOL changed=NO;
 NSRegularExpression *line=[NSRegularExpression regularExpressionWithPattern:@"^EXP_VFXTAG_[^\\r\\n]+" options:NSRegularExpressionAnchorsMatchLines error:nil];
 NSRegularExpression *token=[NSRegularExpression regularExpressionWithPattern:@"\\S+" options:0 error:nil];
 for(NSTextCheckingResult *row in [line matchesInString:out options:0 range:NSMakeRange(0,out.length)].reverseObjectEnumerator){
  NSString *value=[out substringWithRange:row.range];NSArray<NSTextCheckingResult*> *items=[token matchesInString:value options:0 range:NSMakeRange(0,value.length)];
  if(items.count!=21||![tags containsObject:[value substringWithRange:items[0].range]])continue;
  NSMutableString *replacement=[value mutableCopy];
  // Keep primary FX (ground and air), damage/scorch/fire/physics fields untouched.
  for(NSNumber *index in @[@6,@5,@3,@2]){
   NSRange range=items[index.unsignedIntegerValue].range;
   NSString *name=[value substringWithRange:range];if(![name hasPrefix:@"exp_"])continue;
   NSMutableString *blank=[NSMutableString stringWithString:@"-"];while(blank.length<range.length)[blank appendString:@" "];
   [replacement replaceCharactersInRange:range withString:blank];changed=YES;
  }
  [out replaceCharactersInRange:row.range withString:replacement];
 }
 NSData *result=[out dataUsingEncoding:NSUTF8StringEncoding];return changed&&result.length==data.length?result:nil;
}
