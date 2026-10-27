#import <Foundation/Foundation.h>
#include <assert.h>
extern NSData *PGRewriteVisualEffects(NSData*,BOOL,BOOL);
extern NSData *PGRewriteExplosionEffects(NSData*);
int main(){@autoreleasepool{
 NSString *xml=@"<r><light_ray_mult> 1.0000 2.0 </light_ray_mult><Lensflare_visibility> 1 </Lensflare_visibility><particle_light_intensity_mult> 4.0 </particle_light_intensity_mult><damage>100</damage></r>";
 NSData *input=[xml dataUsingEncoding:NSUTF8StringEncoding];
 NSData *data=PGRewriteVisualEffects(input,YES,NO);assert(data.length==input.length);
 NSString *text=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
 assert([text containsString:@"<particle_light_intensity_mult> 4.0 </particle_light_intensity_mult>"]);
 assert([text containsString:@"<damage>100</damage>"]);
 assert([text containsString:@"<light_ray_mult> 0      0   </light_ray_mult>"]);
 assert(PGRewriteVisualEffects(input,NO,YES)==nil);
 assert(PGRewriteVisualEffects(input,NO,NO)==nil);
 NSData *table=[NSData dataWithContentsOfFile:@"../tests/fixtures/explosionfx.dat"];
 assert(table);NSData *patched=PGRewriteExplosionEffects(table);assert(patched&&patched.length==table.length);
 NSString *before=[[NSString alloc] initWithData:table encoding:NSUTF8StringEncoding];
 NSString *after=[[NSString alloc] initWithData:patched encoding:NSUTF8StringEncoding];
 NSArray *oldLines=[before componentsSeparatedByString:@"\n"],*newLines=[after componentsSeparatedByString:@"\n"];assert(oldLines.count==newLines.count);
 NSUInteger changed=0;
 for(NSUInteger i=0;i<oldLines.count;i++){
  if([oldLines[i] isEqualToString:newLines[i]])continue;changed++;
  NSPredicate *notEmpty=[NSPredicate predicateWithFormat:@"length > 0"];
  NSArray *old=[[oldLines[i] componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] filteredArrayUsingPredicate:notEmpty];
  NSArray *new=[[newLines[i] componentsSeparatedByCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet] filteredArrayUsingPredicate:notEmpty];
  assert(old.count==21&&new.count==21);
  for(NSUInteger j=0;j<21;j++)if(j!=2&&j!=3&&j!=5&&j!=6)assert([old[j] isEqualToString:new[j]]);
 }
 assert(changed>0);assert(PGRewriteExplosionEffects(patched)==nil);puts("PASS: explosion-only auxiliary FX; primary FX, fire, damage and other graphics unchanged");
}return 0;}
