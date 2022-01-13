//
//  ViewController.m
//  websocketppIosDemo
//
//  Created by Anton Katekov on 2022/01/13.
//

#import "ViewController.h"
#include "Socket.hpp"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
            Socket s;
            s.Send();
        });
    });
}


@end
