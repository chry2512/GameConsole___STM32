#import <Cocoa/Cocoa.h>

#include "snake.h"
#include "console.h"

static const CGFloat CellSize = 22.0;
static const CGFloat BoardPadding = 18.0;
static const CGFloat ControlsHeight = 112.0;

@class SnakeWindowController;

typedef NS_ENUM(NSInteger, HostScreen) {
    HostScreenPowerOff,
    HostScreenStart,
    HostScreenName,
    HostScreenDifficulty,
    HostScreenGame
};

@interface SnakeWindow : NSWindow
@property(nonatomic, assign) SnakeWindowController *controller;
@end

@interface SnakeView : NSView
@property(nonatomic, assign) Game_t *game;
@property(nonatomic, assign) SnakeWindowController *controller;
- (void)drawSnake:(const Snake_t *)snake
                    originX:(CGFloat)originX
                    originY:(CGFloat)originY
                bodyColor:(NSColor *)bodyColor
                headColor:(NSColor *)headColor;
@end

@interface SnakeWindowController : NSWindowController
@property(nonatomic, assign) Game_t game;
@property(nonatomic, strong) NSTimer *timer;
@property(nonatomic, strong) SnakeView *snakeView;
@property(nonatomic, strong) NSView *contentView;
@property(nonatomic, strong) NSTextField *nameField;
@property(nonatomic, strong) NSButton *restartButton;
@property(nonatomic, strong) NSButton *gameBackButton;
@property(nonatomic, strong) NSView *gameOverOverlay;
@property(nonatomic, strong) NSTextField *gameOverTitle;
@property(nonatomic, strong) NSTextField *gameOverMessage;
@property(nonatomic, strong) NSTextField *leaderboardText;
@property(nonatomic, copy) NSString *playerName;
@property(nonatomic, assign) HostScreen screen;
@property(nonatomic, assign) Console_t console;
- (void)handleKeyEvent:(NSEvent *)event;
- (void)moveUp:(id)sender;
- (void)moveDown:(id)sender;
- (void)moveLeft:(id)sender;
- (void)moveRight:(id)sender;
- (void)togglePause:(id)sender;
- (void)exitGame:(id)sender;
- (void)startGameFromButton:(id)sender;
- (void)startLevel:(id)sender;
- (void)backToStart:(id)sender;
- (NSButton *)buttonWithTitle:(NSString *)title frame:(NSRect)frame action:(SEL)action;
- (NSButton *)levelButtonWithTitle:(NSString *)title frame:(NSRect)frame action:(SEL)action;
- (NSTextField *)labelWithText:(NSString *)text frame:(NSRect)frame size:(CGFloat)size;
- (void)showStartScreen;
- (void)showPowerOffScreen;
- (void)showNameScreen;
- (void)showDifficultyScreen;
- (void)showGameScreen;
- (NSImageView *)backgroundImageViewNamed:(NSString *)fileName;
- (void)togglePower:(id)sender;
- (void)nameKey:(id)sender;
- (void)confirmName:(id)sender;
- (void)restartGame:(id)sender;
- (void)updateGameOverOverlay;
@end

@implementation SnakeWindow

- (BOOL)canBecomeKeyWindow {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    [self.controller handleKeyEvent:event];
}

@end

@implementation SnakeView

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    [self.controller handleKeyEvent:event];
}

- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;

    [[NSColor colorWithCalibratedRed:0.03 green:0.06 blue:0.05 alpha:1.0] setFill];
    NSRectFill(self.bounds);

    CGFloat boardWidth = GRID_WIDTH * CellSize;
    CGFloat boardHeight = GRID_HEIGHT * CellSize;
    CGFloat originX = (self.bounds.size.width - boardWidth) / 2.0;
    CGFloat originY = (self.bounds.size.height - boardHeight) / 2.0;

    [[NSColor colorWithCalibratedRed:0.08 green:0.15 blue:0.12 alpha:1.0] setStroke];
    for (int x = 0; x <= GRID_WIDTH; x++) {
        NSBezierPath *line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(originX + x * CellSize, originY)];
        [line lineToPoint:NSMakePoint(originX + x * CellSize, originY + boardHeight)];
        [line stroke];
    }
    for (int y = 0; y <= GRID_HEIGHT; y++) {
        NSBezierPath *line = [NSBezierPath bezierPath];
        [line moveToPoint:NSMakePoint(originX, originY + y * CellSize)];
        [line lineToPoint:NSMakePoint(originX + boardWidth, originY + y * CellSize)];
        [line stroke];
    }

    [[NSColor colorWithCalibratedRed:0.25 green:0.30 blue:0.32 alpha:1.0] setFill];
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            Point_t point = {(int16_t)x, (int16_t)y};
            if (Snake_IsObstacle(self.game, point)) {
                NSRect wallRect = NSMakeRect(originX + x * CellSize + 1,
                                             originY + (GRID_HEIGHT - 1 - y) * CellSize + 1,
                                             CellSize - 2, CellSize - 2);
                [[NSBezierPath bezierPathWithRoundedRect:wallRect xRadius:4.0 yRadius:4.0] fill];
            }
        }
    }

    if (self.game->state == GAME_STATE_RUNNING ||
        self.game->state == GAME_STATE_PAUSED ||
        self.game->state == GAME_STATE_GAMEOVER) {
        for (uint8_t foodIndex = 0U; foodIndex < self.game->foodCount; foodIndex++) {
            Point_t food = self.game->foods[foodIndex];
            NSRect foodRect = NSMakeRect(originX + food.x * CellSize + 4,
                                         originY + (GRID_HEIGHT - 1 - food.y) * CellSize + 4,
                                         CellSize - 8, CellSize - 8);
            [[NSColor colorWithCalibratedRed:0.92 green:0.12 blue:0.10 alpha:1.0] setFill];
            [[NSBezierPath bezierPathWithOvalInRect:foodRect] fill];
            [[NSColor colorWithCalibratedRed:0.35 green:0.18 blue:0.06 alpha:1.0] setStroke];
            NSBezierPath *stem = [NSBezierPath bezierPath];
            [stem moveToPoint:NSMakePoint(NSMidX(foodRect), NSMaxY(foodRect))];
            [stem lineToPoint:NSMakePoint(NSMidX(foodRect) + 2.0, NSMaxY(foodRect) + 5.0)];
            [stem setLineWidth:2.0];
            [stem stroke];
        }

        [self drawSnake:&self.game->snake
                originX:originX
                originY:originY
              bodyColor:[NSColor colorWithCalibratedRed:0.35 green:0.75 blue:0.25 alpha:1.0]
              headColor:[NSColor colorWithCalibratedRed:0.85 green:1.0 blue:0.47 alpha:1.0]];
        NSArray<NSColor *> *enemyBodyColors = @[
            [NSColor colorWithCalibratedRed:0.85 green:0.28 blue:0.08 alpha:1.0],
            [NSColor colorWithCalibratedRed:0.05 green:0.35 blue:0.85 alpha:1.0],
            [NSColor colorWithCalibratedRed:0.75 green:0.12 blue:0.75 alpha:1.0],
            [NSColor colorWithCalibratedRed:0.82 green:0.68 blue:0.05 alpha:1.0]
        ];
        NSArray<NSColor *> *enemyHeadColors = @[
            [NSColor colorWithCalibratedRed:1.0 green:0.62 blue:0.16 alpha:1.0],
            [NSColor colorWithCalibratedRed:0.20 green:0.75 blue:1.0 alpha:1.0],
            [NSColor colorWithCalibratedRed:1.0 green:0.35 blue:0.85 alpha:1.0],
            [NSColor colorWithCalibratedRed:1.0 green:0.90 blue:0.25 alpha:1.0]
        ];
        for (uint8_t enemyIndex = 0U; enemyIndex < self.game->enemyCount; enemyIndex++) {
            [self drawSnake:&self.game->enemies[enemyIndex]
                    originX:originX
                    originY:originY
                  bodyColor:enemyBodyColors[enemyIndex]
                  headColor:enemyHeadColors[enemyIndex]];
        }
    }

    NSString *status = nil;
    if (self.game->state == GAME_STATE_IDLE) status = @"GAME OVER";
    if (self.game->state == GAME_STATE_PAUSED) status = @"PAUSA";
    if (self.game->state == GAME_STATE_LEVEL_TRANSITION) {
        status = [NSString stringWithFormat:@"STAGE %u - Caricamento Livello", self.game->stage];
    }
    if (self.game->state == GAME_STATE_GAMEOVER) {
        status = [NSString stringWithFormat:@"GAME OVER - Punteggio: %u", self.game->score];
    }
    if (self.game->state == GAME_STATE_VICTORY) {
        status = self.game->easterEggUnlocked
            ? @"HAI VINTO\n  EASTER EGG -> PLAYER NAME = CHRY\n ANGOLO DESTRO IN BASSO"
            : @"HAI VINTO - Livello 100";
    }
    if (status != nil) {
        BOOL victoryStatus = self.game->state == GAME_STATE_VICTORY;
        NSDictionary *attributes = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:victoryStatus ? 16.0 : 22.0],
            NSForegroundColorAttributeName: [NSColor whiteColor]
        };
        if (victoryStatus) {
            NSMutableParagraphStyle *paragraph = [[NSMutableParagraphStyle alloc] init];
            paragraph.alignment = NSTextAlignmentCenter;
            NSMutableDictionary *centeredAttributes = [attributes mutableCopy];
            centeredAttributes[NSParagraphStyleAttributeName] = paragraph;
            [status drawInRect:NSMakeRect(12.0, self.bounds.size.height - 58.0,
                                          self.bounds.size.width - 24.0, 46.0)
                withAttributes:centeredAttributes];
        } else {
            NSSize textSize = [status sizeWithAttributes:attributes];
            [status drawAtPoint:NSMakePoint((self.bounds.size.width - textSize.width) / 2.0,
                                            self.bounds.size.height - 42.0)
                 withAttributes:attributes];
        }
    }

    if (self.game->state == GAME_STATE_RUNNING || self.game->state == GAME_STATE_PAUSED) {
        NSString *hud = [NSString stringWithFormat:@"Player Name: %@    Punteggio: %u    Stage: %u",
                 self.controller.playerName ?: @"Serpente", self.game->score, self.game->stage];
        NSDictionary *hudAttributes = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:16.0],
            NSForegroundColorAttributeName: [NSColor whiteColor]
        };
        [hud drawAtPoint:NSMakePoint(18.0, self.bounds.size.height - 26.0)
          withAttributes:hudAttributes];
    }

    if (self.game->state == GAME_STATE_LEVEL_TRANSITION) {
        NSRect fogRect = NSMakeRect(0.0, 0.0, self.bounds.size.width, self.bounds.size.height);
        [[NSColor colorWithCalibratedWhite:0.05 alpha:0.72] setFill];
        NSRectFillUsingOperation(fogRect, NSCompositingOperationSourceOver);
        NSDictionary *transitionAttributes = @{
            NSFontAttributeName: [NSFont boldSystemFontOfSize:22.0],
            NSForegroundColorAttributeName: [NSColor whiteColor]
        };
        NSString *transitionText = [NSString stringWithFormat:@"STAGE %u\nPreparazione Livello...", self.game->stage];
        NSMutableParagraphStyle *paragraph = [[NSMutableParagraphStyle alloc] init];
        paragraph.alignment = NSTextAlignmentCenter;
        NSMutableDictionary *centeredAttributes = [transitionAttributes mutableCopy];
        centeredAttributes[NSParagraphStyleAttributeName] = paragraph;
        [transitionText drawInRect:NSMakeRect(0.0, self.bounds.size.height / 2.0 - 28.0,
                                               self.bounds.size.width, 60.0)
                    withAttributes:centeredAttributes];
    }
}

- (void)drawSnake:(const Snake_t *)snake
          originX:(CGFloat)originX
          originY:(CGFloat)originY
        bodyColor:(NSColor *)bodyColor
        headColor:(NSColor *)headColor {
    if (snake == NULL || snake->length == 0U) return;

    NSShadow *shadow = [[NSShadow alloc] init];
    shadow.shadowColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.35];
    shadow.shadowBlurRadius = 2.0;
    shadow.shadowOffset = NSMakeSize(0.0, -1.0);

    for (uint16_t i = snake->length; i > 0U; i--) {
        Point_t segment = snake->body[i - 1U];
        CGFloat inset = i == 1U ? 1.0 : 3.0;
        NSRect segmentRect = NSMakeRect(originX + segment.x * CellSize + inset,
                                        originY + (GRID_HEIGHT - 1 - segment.y) * CellSize + inset,
                                        CellSize - inset * 2.0,
                                        CellSize - inset * 2.0);
        [i == 1U ? headColor : bodyColor setFill];
        [shadow set];
        [[NSBezierPath bezierPathWithOvalInRect:segmentRect] fill];
    }

    Point_t head = snake->body[0];
    CGFloat centerX = originX + head.x * CellSize + CellSize / 2.0;
    CGFloat centerY = originY + (GRID_HEIGHT - 1 - head.y) * CellSize + CellSize / 2.0;
    CGFloat forwardX = 0.0;
    CGFloat forwardY = 0.0;
    if (snake->dir == DIR_UP) forwardY = 1.0;
    else if (snake->dir == DIR_DOWN) forwardY = -1.0;
    else if (snake->dir == DIR_LEFT) forwardX = -1.0;
    else forwardX = 1.0;

    CGFloat sideX = -forwardY;
    CGFloat sideY = forwardX;

    [headColor setFill];
    NSRect muzzleRect = NSMakeRect(centerX + forwardX * 5.0 - 5.0,
                                   centerY + forwardY * 5.0 - 5.0,
                                   10.0, 10.0);
    [[NSBezierPath bezierPathWithOvalInRect:muzzleRect] fill];

    [[NSColor blackColor] setFill];
    for (CGFloat side = -1.0; side <= 1.0; side += 2.0) {
        NSRect eyeRect = NSMakeRect(centerX + forwardX * 4.0 + sideX * side * 4.0 - 2.0,
                                    centerY + forwardY * 4.0 + sideY * side * 4.0 - 2.0,
                                    4.0, 4.0);
        [[NSBezierPath bezierPathWithOvalInRect:eyeRect] fill];
    }

    [[NSColor colorWithCalibratedRed:0.95 green:0.18 blue:0.22 alpha:1.0] setStroke];
    NSBezierPath *tongue = [NSBezierPath bezierPath];
    NSPoint tongueStart = NSMakePoint(centerX + forwardX * 8.0, centerY + forwardY * 8.0);
    NSPoint tongueEnd = NSMakePoint(centerX + forwardX * 13.0, centerY + forwardY * 13.0);
    [tongue moveToPoint:tongueStart];
    [tongue lineToPoint:tongueEnd];
    [tongue setLineWidth:1.5];
    [tongue stroke];
}

@end

@implementation SnakeWindowController

- (instancetype)init {
    CGFloat width = GRID_WIDTH * CellSize + BoardPadding * 2;
    CGFloat boardHeight = GRID_HEIGHT * CellSize + BoardPadding * 2 + 48;
    CGFloat height = boardHeight + ControlsHeight;
    NSRect frame = NSMakeRect(0, 0, width, height);
        SnakeWindow *window = [[SnakeWindow alloc] initWithContentRect:frame
                                                                                                                     styleMask:NSWindowStyleMaskTitled |
                                                                                                                                         NSWindowStyleMaskClosable |
                                                                                                                                         NSWindowStyleMaskMiniaturizable
                                                                                                                         backing:NSBackingStoreBuffered
                                                                                                                             defer:NO];
    self = [super initWithWindow:window];
    if (self) {
        self.window.title = @"Snake C test";
        self.window.releasedWhenClosed = NO;
        self.game = (Game_t){0};
        Console_Init(&_console, &_game);
        self.playerName = @"Serpente";
        window.controller = self;

        self.snakeView = [[SnakeView alloc] initWithFrame:NSMakeRect(0, ControlsHeight, width, boardHeight)];
        self.snakeView.game = &_game;
        self.snakeView.controller = self;

        self.contentView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
        self.window.contentView = self.contentView;
        [self showPowerOffScreen];

        __weak SnakeWindowController *weakSelf = self;
        self.timer = [NSTimer scheduledTimerWithTimeInterval:0.02
                                                       repeats:YES
                                                         block:^(NSTimer *timer) {
            (void)timer;
            SnakeWindowController *controller = weakSelf;
            static NSTimeInterval elapsed = 0.0;
            elapsed += 0.02;
            if (controller.game.state == GAME_STATE_RUNNING &&
                elapsed >= (double)Snake_GetMoveIntervalMs(&controller->_game) / 1000.0) {
                Snake_Update(&controller->_game);
                elapsed = 0.0;
            }
            if (controller.game.state == GAME_STATE_LEVEL_TRANSITION) {
                Snake_AdvanceTransition(&controller->_game, 20U);
            }
            if (controller.game.state != GAME_STATE_RUNNING) elapsed = 0.0;
            [controller updateGameOverOverlay];
            [controller.snakeView setNeedsDisplay:YES];
        }];
    }
    return self;
}

- (NSButton *)buttonWithTitle:(NSString *)title frame:(NSRect)frame action:(SEL)action {
    NSButton *button = [[NSButton alloc] initWithFrame:frame];
    button.title = title;
    button.bezelStyle = NSBezelStyleRounded;
    button.target = self;
    button.action = action;
    return button;
}

- (NSButton *)startButtonWithFrame:(NSRect)frame action:(SEL)action {
    NSButton *button = [self buttonWithTitle:@"INIZIA" frame:frame action:action];
    button.font = [NSFont boldSystemFontOfSize:20.0];
    button.bezelStyle = NSBezelStyleRounded;
    button.bordered = NO;
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.18 green:0.68 blue:0.25 alpha:1.0].CGColor;
    button.layer.cornerRadius = 18.0;
    button.layer.borderWidth = 3.0;
    button.layer.borderColor = [NSColor colorWithCalibratedRed:0.08 green:0.35 blue:0.12 alpha:1.0].CGColor;
    return button;
}

- (NSButton *)levelButtonWithTitle:(NSString *)title frame:(NSRect)frame action:(SEL)action {
    NSButton *button = [self buttonWithTitle:title frame:frame action:action];
    button.font = [NSFont boldSystemFontOfSize:17.0];
    button.bezelStyle = NSBezelStyleRounded;
    button.bordered = NO;
    button.contentTintColor = [NSColor colorWithCalibratedRed:0.05 green:0.25 blue:0.08 alpha:1.0];
    button.wantsLayer = YES;
    button.layer.backgroundColor = [NSColor colorWithCalibratedRed:0.48 green:0.88 blue:0.36 alpha:1.0].CGColor;
    button.layer.cornerRadius = 16.0;
    button.layer.borderWidth = 3.0;
    button.layer.borderColor = [NSColor colorWithCalibratedRed:0.18 green:0.48 blue:0.14 alpha:1.0].CGColor;
    return button;
}

- (NSTextField *)labelWithText:(NSString *)text frame:(NSRect)frame size:(CGFloat)size {
    NSTextField *label = [[NSTextField alloc] initWithFrame:frame];
    label.stringValue = text;
    label.alignment = NSTextAlignmentCenter;
    label.font = [NSFont boldSystemFontOfSize:size];
    label.textColor = [NSColor whiteColor];
    label.drawsBackground = NO;
    label.bordered = NO;
    label.editable = NO;
    return label;
}

- (NSImageView *)backgroundImageViewNamed:(NSString *)fileName {
    NSString *executableDirectory = [[[NSBundle mainBundle] executablePath] stringByDeletingLastPathComponent];
    NSString *workingDirectoryPath = [[NSFileManager defaultManager] currentDirectoryPath];
    NSArray<NSString *> *imagePaths = @[
        [workingDirectoryPath stringByAppendingPathComponent:[@"asset" stringByAppendingPathComponent:fileName]],
        [executableDirectory stringByAppendingPathComponent:[@"asset" stringByAppendingPathComponent:fileName]]
    ];
    NSImage *image = nil;
    for (NSString *path in imagePaths) {
        image = [[NSImage alloc] initWithContentsOfFile:path];
        if (image != nil) break;
    }
    NSImageView *imageView = [[NSImageView alloc] initWithFrame:self.contentView.bounds];
    imageView.image = image;
    imageView.imageScaling = NSImageScaleProportionallyUpOrDown;
    imageView.imageAlignment = NSImageAlignCenter;
    imageView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    return imageView;
}

- (void)clearContentView {
    for (NSView *subview in [self.contentView.subviews copy]) {
        [subview removeFromSuperview];
    }
}

- (void)showStartScreen {
    self.screen = HostScreenStart;
    [self clearContentView];
    CGFloat width = self.contentView.bounds.size.width;
    CGFloat height = self.contentView.bounds.size.height;
    [self.contentView addSubview:[self backgroundImageViewNamed:@"snakeSfondo.jpeg"] positioned:NSWindowBelow relativeTo:nil];
    [self.contentView addSubview:[self startButtonWithFrame:NSMakeRect(width / 2.0 - 92.0, height / 2.0 - 192.0, 184.0, 76.0) action:@selector(startGameFromButton:)]];
}

- (void)showPowerOffScreen {
    self.screen = HostScreenPowerOff;
    [self clearContentView];
    [self.contentView addSubview:[self backgroundImageViewNamed:@"cubeniro.jpeg"] positioned:NSWindowBelow relativeTo:nil];
    [self.contentView addSubview:[self buttonWithTitle:@"ON / OFF" frame:NSMakeRect(18.0, 12.0, 84.0, 30.0) action:@selector(togglePower:)]];
}

- (void)showNameScreen {
    self.screen = HostScreenName;
    [self clearContentView];
    CGFloat width = self.contentView.bounds.size.width;
    CGFloat height = self.contentView.bounds.size.height;
    [self.contentView addSubview:[self backgroundImageViewNamed:@"snakeSfondo.jpeg"] positioned:NSWindowBelow relativeTo:nil];
    NSView *dimOverlay = [[NSView alloc] initWithFrame:self.contentView.bounds];
    dimOverlay.wantsLayer = YES;
    dimOverlay.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.52].CGColor;
    dimOverlay.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.contentView addSubview:dimOverlay];
    [self.contentView addSubview:[self labelWithText:@"Player Name" frame:NSMakeRect(0, height / 2.0 + 120.0, width, 34.0) size:25.0]];
    self.nameField = [[NSTextField alloc] initWithFrame:NSMakeRect(width / 2.0 - 145.0, height / 2.0 + 70.0, 290.0, 38.0)];
    self.nameField.stringValue = @"";
    self.nameField.alignment = NSTextAlignmentCenter;
    self.nameField.font = [NSFont systemFontOfSize:21.0];
    self.nameField.editable = NO;
    [self.contentView addSubview:self.nameField];

    NSString *letters = @"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    CGFloat buttonWidth = 34.0;
    CGFloat buttonHeight = 30.0;
    CGFloat gap = 4.0;
    CGFloat keyboardWidth = 10.0 * buttonWidth + 9.0 * gap;
    CGFloat startX = (width - keyboardWidth) / 2.0;
    CGFloat startY = height / 2.0 + 12.0;
    for (NSUInteger index = 0; index < letters.length; index++) {
        NSUInteger row = index / 10U;
        NSUInteger column = index % 10U;
        NSString *letter = [letters substringWithRange:NSMakeRange(index, 1)];
        [self.contentView addSubview:[self buttonWithTitle:letter
                                                       frame:NSMakeRect(startX + column * (buttonWidth + gap),
                                                                        startY - row * (buttonHeight + gap),
                                                                        buttonWidth, buttonHeight)
                                                      action:@selector(nameKey:)]];
    }
    CGFloat commandY = startY - 3.0 * (buttonHeight + gap);
    [self.contentView addSubview:[self buttonWithTitle:@"Cancella" frame:NSMakeRect(startX + 20.0, commandY, 120.0, 34.0) action:@selector(nameKey:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"Conferma" frame:NSMakeRect(startX + 216.0, commandY, 120.0, 34.0) action:@selector(confirmName:)]];
}

- (void)showDifficultyScreen {
    self.screen = HostScreenDifficulty;
    [self clearContentView];
    CGFloat width = self.contentView.bounds.size.width;
    CGFloat height = self.contentView.bounds.size.height;
    [self.contentView addSubview:[self backgroundImageViewNamed:@"snakeSfondo.jpeg"] positioned:NSWindowBelow relativeTo:nil];
    NSView *dimOverlay = [[NSView alloc] initWithFrame:self.contentView.bounds];
    dimOverlay.wantsLayer = YES;
    dimOverlay.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.52].CGColor;
    dimOverlay.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.contentView addSubview:dimOverlay];
    CGFloat buttonWidth = 130.0;
    CGFloat gap = 14.0;
    CGFloat startX = (width - (buttonWidth * 3.0 + gap * 2.0)) / 2.0;
    CGFloat buttonY = height / 2.0 - 100.0;
    [self.contentView addSubview:[self levelButtonWithTitle:@"Facile" frame:NSMakeRect(startX, buttonY, buttonWidth, 52.0) action:@selector(startLevel:)]];
    [self.contentView addSubview:[self levelButtonWithTitle:@"Medio" frame:NSMakeRect(startX + buttonWidth + gap, buttonY, buttonWidth, 52.0) action:@selector(startLevel:)]];
    [self.contentView addSubview:[self levelButtonWithTitle:@"Difficile" frame:NSMakeRect(startX + (buttonWidth + gap) * 2.0, buttonY, buttonWidth, 52.0) action:@selector(startLevel:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"Indietro" frame:NSMakeRect(width / 2.0 - 55.0, 32.0, 110.0, 32.0) action:@selector(backToStart:)]];
}

- (void)showGameScreen {
    self.screen = HostScreenGame;
    [self clearContentView];
    CGFloat width = self.contentView.bounds.size.width;
    CGFloat height = self.contentView.bounds.size.height;
    [self.contentView addSubview:self.snakeView];
    [self.snakeView setFrame:NSMakeRect(0, ControlsHeight, width, height - ControlsHeight)];

    CGFloat centerX = width / 2.0;
    CGFloat buttonWidth = 92.0;
    CGFloat buttonHeight = 30.0;
    CGFloat rowY = 54.0;
    [self.contentView addSubview:[self buttonWithTitle:@"▲" frame:NSMakeRect(centerX - buttonWidth / 2.0, rowY + 32.0, buttonWidth, buttonHeight) action:@selector(moveUp:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"◀" frame:NSMakeRect(centerX - buttonWidth * 1.5 - 8.0, rowY, buttonWidth, buttonHeight) action:@selector(moveLeft:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"▼" frame:NSMakeRect(centerX - buttonWidth / 2.0, rowY, buttonWidth, buttonHeight) action:@selector(moveDown:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"▶" frame:NSMakeRect(centerX + buttonWidth / 2.0 + 8.0, rowY, buttonWidth, buttonHeight) action:@selector(moveRight:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"Indietro" frame:NSMakeRect(112.0, 12.0, 100.0, buttonHeight) action:@selector(exitGame:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"Pause" frame:NSMakeRect(width - 190.0, 12.0, 84.0, buttonHeight) action:@selector(togglePause:)]];
    [self.contentView addSubview:[self buttonWithTitle:@"ON / OFF" frame:NSMakeRect(18.0, 12.0, 84.0, buttonHeight) action:@selector(togglePower:)]];

    self.gameOverOverlay = [[NSView alloc] initWithFrame:NSMakeRect(50.0, ControlsHeight + 28.0, width - 100.0, 340.0)];
    self.gameOverOverlay.wantsLayer = YES;
    self.gameOverOverlay.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.03 alpha:0.94].CGColor;
    self.gameOverOverlay.layer.cornerRadius = 14.0;
    self.gameOverTitle = [self labelWithText:@"GAME OVER" frame:NSMakeRect(0.0, 292.0, self.gameOverOverlay.bounds.size.width, 36.0) size:28.0];
    self.gameOverMessage = [self labelWithText:@"Punteggio finale" frame:NSMakeRect(0.0, 258.0, self.gameOverOverlay.bounds.size.width, 24.0) size:15.0];
    self.leaderboardText = [self labelWithText:@"CLASSIFICA" frame:NSMakeRect(24.0, 72.0, self.gameOverOverlay.bounds.size.width - 48.0, 166.0) size:13.0];
    self.leaderboardText.alignment = NSTextAlignmentLeft;
    self.leaderboardText.font = [NSFont monospacedSystemFontOfSize:13.0 weight:NSFontWeightRegular];
    self.leaderboardText.lineBreakMode = NSLineBreakByClipping;
    [self.gameOverOverlay addSubview:self.gameOverTitle];
    [self.gameOverOverlay addSubview:self.gameOverMessage];
    [self.gameOverOverlay addSubview:self.leaderboardText];
    self.restartButton = [self buttonWithTitle:@"Restart" frame:NSMakeRect(32.0, 22.0, 130.0, 38.0) action:@selector(restartGame:)];
    self.gameBackButton = [self buttonWithTitle:@"Indietro" frame:NSMakeRect(self.gameOverOverlay.bounds.size.width - 162.0, 22.0, 130.0, 38.0) action:@selector(exitGame:)];
    [self.gameOverOverlay addSubview:self.restartButton];
    [self.gameOverOverlay addSubview:self.gameBackButton];
    [self.contentView addSubview:self.gameOverOverlay];
    [self updateGameOverOverlay];
}

- (void)startGame {
    [self showNameScreen];
}

- (void)startLevel:(id)sender {
    NSString *title = [(NSButton *)sender title];
    Level_t level = LEVEL_EASY;
    ConsoleEvent_t event = CONSOLE_EVENT_LEVEL_EASY;
    if ([title isEqualToString:@"Medio"]) {
        level = LEVEL_MEDIUM;
        event = CONSOLE_EVENT_LEVEL_MEDIUM;
    } else if ([title isEqualToString:@"Difficile"]) {
        level = LEVEL_HARD;
        event = CONSOLE_EVENT_LEVEL_HARD;
    }
    (void)level;
    Console_HandleEvent(&_console, &_game, event, '\0');
    [self showGameScreen];
}

- (void)togglePower:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_POWER, '\0');
    if (Console_GetState(&_console) == CONSOLE_STATE_OFF) [self showPowerOffScreen];
    else [self showStartScreen];
}

- (void)updateGameOverOverlay {
    BOOL visible = self.screen == HostScreenGame &&
                   (self.game.state == GAME_STATE_GAMEOVER ||
                    self.game.state == GAME_STATE_VICTORY);
    self.gameOverOverlay.hidden = !visible;
    if (visible) {
        BOOL victory = self.game.state == GAME_STATE_VICTORY;
        self.gameOverTitle.stringValue = victory ? @"HAI VINTO!" : @"GAME OVER";
        self.gameOverMessage.stringValue = victory
            ? [NSString stringWithFormat:@"Punteggio finale: %u  |  Easter Egg attivo", self.game.score]
            : [NSString stringWithFormat:@"Punteggio finale: %u", self.game.score];
        NSMutableString *ranking = [NSMutableString stringWithString:@"CLASSIFICA\n"];
        uint8_t count = Snake_Leaderboard_Count();
        for (uint8_t index = 0U; index < SNAKE_LEADERBOARD_SIZE; index++) {
            const LeaderboardEntry_t *entry = Snake_Leaderboard_Get(index);
            if (index < count && entry != NULL) {
                [ranking appendFormat:@"%2u. %-12s  %6u\n", index + 1U, entry->playerName, entry->score];
            } else {
                [ranking appendFormat:@"%2u. %-12@  %6@\n", index + 1U, @"---", @"---"];
            }
        }
        self.leaderboardText.stringValue = ranking;
    }
}

- (void)nameKey:(id)sender {
    NSString *key = [(NSButton *)sender title];
    if ([key isEqualToString:@"Cancella"]) {
        Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_NAME_BACKSPACE, '\0');
    } else if (key.length == 1U) {
        Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_NAME_CHAR, key.UTF8String[0]);
    }
    self.nameField.stringValue = [NSString stringWithUTF8String:Snake_GetPlayerName(&_game)];
}

- (void)confirmName:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_NAME_CONFIRM, '\0');
    self.playerName = [NSString stringWithUTF8String:Snake_GetPlayerName(&_game)];
    [self showDifficultyScreen];
}

- (void)setDirection:(Direction_t)direction {
    ConsoleEvent_t event = CONSOLE_EVENT_UP;
    if (direction == DIR_DOWN) event = CONSOLE_EVENT_DOWN;
    else if (direction == DIR_LEFT) event = CONSOLE_EVENT_LEFT;
    else if (direction == DIR_RIGHT) event = CONSOLE_EVENT_RIGHT;
    Console_HandleEvent(&_console, &_game, event, '\0');
    [self.snakeView setNeedsDisplay:YES];
}

- (void)moveUp:(id)sender {
    (void)sender;
    [self setDirection:DIR_UP];
}

- (void)moveDown:(id)sender {
    (void)sender;
    [self setDirection:DIR_DOWN];
}

- (void)moveLeft:(id)sender {
    (void)sender;
    [self setDirection:DIR_LEFT];
}

- (void)moveRight:(id)sender {
    (void)sender;
    [self setDirection:DIR_RIGHT];
}

- (void)togglePause:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_PAUSE, '\0');
    [self.snakeView setNeedsDisplay:YES];
}

- (void)exitGame:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_BACK, '\0');
    [self showStartScreen];
}

- (void)restartGame:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_RESTART, '\0');
    [self updateGameOverOverlay];
    [self.snakeView setNeedsDisplay:YES];
}

- (void)startGameFromButton:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_X, '\0');
    [self showNameScreen];
}

- (void)backToStart:(id)sender {
    (void)sender;
    Console_HandleEvent(&_console, &_game, CONSOLE_EVENT_BACK, '\0');
    [self showStartScreen];
}

- (void)handleKeyEvent:(NSEvent *)event {
    NSString *key = event.charactersIgnoringModifiers.lowercaseString;
    Direction_t direction;
    BOOL hasDirection = YES;

    if (event.keyCode == 126) direction = DIR_UP;
    else if (event.keyCode == 125) direction = DIR_DOWN;
    else if (event.keyCode == 123) direction = DIR_LEFT;
    else if (event.keyCode == 124) direction = DIR_RIGHT;
    else hasDirection = NO;

    if (hasDirection) {
        [self setDirection:direction];
    } else if ([key isEqualToString:@"x"]) {
        [self exitGame:nil];
    } else {
        [super keyDown:event];
    }
    [self.snakeView setNeedsDisplay:YES];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) SnakeWindowController *controller;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    self.controller = [[SnakeWindowController alloc] init];
    [self.controller showWindow:nil];
    [self.controller.window makeKeyAndOrderFront:nil];
    [self.controller.window makeFirstResponder:self.controller.snakeView];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return YES;
}

@end

int main(void) {
    @autoreleasepool {
        NSApplication *application = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        application.delegate = delegate;
        [application run];
    }
    return 0;
}