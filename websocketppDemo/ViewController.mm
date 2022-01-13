//
//  ViewController.m
//  websocketppDemo
//
//  Created by Anton Katekov on 2022/01/13.
//

#import "ViewController.h"
#include "Socket.hpp"

//#include <string>
//#include <stack>
//class Solution {
//public:
//
//    static bool isValid(std::string s) {
//        if(s.length() == 0) { // use length not size 
//            return true;
//        }
//        std::stack<char> st;
//        for (int i=0; i < s.length(); i++) {
//            if (s[i]=='('||s[i]=='{'||s[i]=='[') {
//                st.push(s[i]);
//            }
//            else if ((s[i]==')'||s[i]=='}'||s[i]==']') && !st.empty())
//            {
//                char cur=st.top();
//                if ((cur=='('&&s[i]==')')||(cur=='{'&&s[i]=='}')||(cur=='['&&s[i]==']')) { 
//                    st.pop();
//                }
//               else 
//                   return false;            
//            }
//            else
//                return false;
//        }
//        return st.empty();
//    }
//};

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
    
//    assert(Solution::isValid("[]") == true);
//    assert(Solution::isValid("[]{}()") == true);
//    assert(Solution::isValid("[]{{[()]}}()") == true);
//    assert(Solution::isValid("[]{{[()]}()") == false);
//    assert(Solution::isValid("]{{[()]}}()") == false);
    
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
            Socket s;
            s.Send();
        });
    });
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}


@end
