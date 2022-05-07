# Twitch Controls Chaos user interface
# 
import PySimpleGUI as sg

dark_mode = True

# base64 strings for button images
toggle_btn_off = b'iVBORw0KGgoAAAANSUhEUgAAACgAAAAoCAYAAACM/rhtAAAABmJLR0QA/wD/AP+gvaeTAAAED0lEQVRYCe1WTWwbRRR+M/vnv9hO7BjHpElMKSlpqBp6gRNHxAFVcKM3qgohQSqoqhQ45YAILUUVDRxAor2VAweohMSBG5ciodJUSVqa/iikaePEP4nj2Ovdnd1l3qqJksZGXscVPaylt7Oe/d6bb9/svO8BeD8vA14GvAx4GXiiM0DqsXv3xBcJU5IO+RXpLQvs5yzTijBmhurh3cyLorBGBVokQG9qVe0HgwiXLowdy9aKsY3g8PA5xYiQEUrsk93JTtjd1x3siIZBkSWQudUK4nZO1w3QuOWXV+HuP/fL85klAJuMCUX7zPj4MW1zvC0Ej4yMp/w++K2rM9b70sHBYCjo34x9bPelsgp/XJksZ7KFuwZjr3732YcL64ttEDw6cq5bVuCvgy/sje7rT0sI8PtkSHSEIRIKgCQKOAUGM6G4VoGlwiqoVd2Za9Vl8u87bGJqpqBqZOj86eEHGNch+M7otwHJNq4NDexJD+59RiCEQG8qzslFgN8ibpvZNsBifgXmFvJg459tiOYmOElzYvr2bbmkD509e1ylGEZk1Y+Ssfan18n1p7vgqVh9cuiDxJPxKPT3dfGXcN4Tp3dsg/27hUQs0qMGpRMYjLz38dcxS7Dm3nztlUAb38p0d4JnLozPGrbFfBFm79c8hA3H2AxcXSvDz7/+XtZE1kMN23hjV7LTRnKBh9/cZnAj94mOCOD32gi2EUw4FIRUMm6LGhyiik86nO5NBdGRpxYH14bbjYfJteN/OKR7UiFZVg5T27QHYu0RBxoONV9W8KQ7QVp0iXdE8fANUGZa0QAvfhhXlkQcmjJZbt631oIBnwKmacYoEJvwiuFgWncWnXAtuVBBEAoVVXWCaQZzxmYuut68b631KmoVBEHMUUrJjQLXRAQVSxUcmrKVHfjWWjC3XOT1FW5QrWpc5IJdQhDKVzOigEqS5dKHMVplnNOqrmsXqUSkn+YzWaHE9RW1FeXL7SKZXBFUrXW6jIV6YTEvMAUu0W/G3kcxPXP5ylQZs4fa6marcWvvZfJu36kuHjlc/nMSuXz+/ejxgqPFpuQ/xVude9eu39Jxu27OLvBGoMjrUN04zrNMbgVmOBZ96iPdPZmYntH5Ls76KuxL9NyoLA/brav7n382emDfHqeooXyhQmARVhSnAwNNMx5bu3V1+habun5nWdXhwJZ2C5mirTesyUR738sv7g88UQ0rEkTDlp+1wwe8Pf0klegUenYlgyg7bby75jUTITs2rhCAXXQ2vwxz84vlB0tZ0wL4NEcLX/04OrrltG1s8aOrHhk51SaK0us+n/K2xexBxljcsm1n6x/Fuv1PCWGiKOaoQCY1Vb9gWPov50+fdEqd21ge3suAlwEvA14G/ucM/AuppqNllLGPKwAAAABJRU5ErkJggg=='
toggle_btn_on = b'iVBORw0KGgoAAAANSUhEUgAAACgAAAAoCAYAAACM/rhtAAAABmJLR0QA/wD/AP+gvaeTAAAD+UlEQVRYCe1XzW8bVRCffbvrtbP+2NhOD7GzLm1VoZaPhvwDnKBUKlVyqAQ3/gAkDlWgPeVQEUCtEOIP4AaHSI0CqBWCQyXOdQuRaEFOk3g3IMWO46+tvZ+PeZs6apq4ipON1MNafrvreTPzfvub92bGAOEnZCBkIGQgZOClZoDrh25y5pdjruleEiX+A+rCaQo05bpuvJ/+IHJCSJtwpAHA/e269g8W5RbuzF6o7OVjF8D3Pr4tSSkyjcqfptPDMDKSleW4DKIggIAD5Yf+Oo4DNg6jbUBlvWLUNutAwZu1GnDjzrcXzGcX2AHw/emFUV6Sfk0pqcKpEydkKSo9q3tkz91uF5aWlo1Gs/mYc+i7tz4//19vsW2AU9O381TiioVCQcnlRsWeQhD3bJyH1/MiFLICyBHiuzQsD1arDvypW7DR9nzZmq47q2W95prm+I9fXfqXCX2AF2d+GhI98Y8xVX0lnxvl2UQQg0csb78ag3NjEeD8lXZ7pRTgftmCu4864OGzrq+5ZU0rCa3m+NzXlzvoAoB3+M+SyWQuaHBTEzKMq/3BMbgM+FuFCDBd9kK5XI5PJBKqLSev+POTV29lKB8rT0yMD0WjUSYLZLxzNgZvIHODOHuATP72Vwc6nQ4Uiw8MUeBU4nHS5HA6TYMEl02wPRcZBJuv+ya+UCZOIBaLwfCwQi1Mc4QXhA+PjWRkXyOgC1uIhW5Qd8yG2TK7kSweLcRGKKVnMNExWWBDTQsH9qVmtmzjiThQDs4Qz/OUSGTwcLwIQTLW58i+yOjpXDLqn1tgmDzXzRCk9eDenjo9yhvBmlizrB3V5dDrNTuY0A7opdndStqmaQLPC1WCGfShYRgHdLe32UrV3ntiH9LliuNrsToNlD4kruN8v75eafnSgC6Luo2+B3fGKskilj5muV6pNhk2Qqg5v7lZ51nBZhNBjGrbxfI1+La5t2JCzfD8RF1HTBGJXyDzs1MblONulEqPDVYXgwDIfNx91IUVbAbY837GMur+/k/XZ75UWmJ77ou5mfM1/0x7vP1ls9XQdF2z9uNsPzosXPNFA5m0/EX72TBSiqsWzN8z/GZB08pWq9VeEZ+0bjKb7RTD2i1P4u6r+bwypo5tZUumEcDAmuC3W8ezIqSGfE6g/sTd1W5p5bKjaWubrmWd29Fu9TD0GlYlmTx+8tTJoZeqYe2BZC1/JEU+wQR5TVEUPptJy3Fs+Vkzgf8lemqHumP1AnYoMZSwsVEz6o26i/G9Lgitb+ZmLu/YZtshfn5FZDPBCcJFQRQ+8ih9DctOFvdLIKHH6uUQnq9yhFu0bec7znZ+xpAGmuqef5/wd8hAyEDIQMjAETHwP7nQl2WnYk4yAAAAAElFTkSuQmCC'

# Theme to immitate Twitch

if dark_mode:
    sg.theme('Dark')
else:
    sg.theme('Default')

class BtnInfo:
    def __init__(self, state=False):
        self.state = state

# Main Monitoring Screen for Streamer
# Vote timer
# Active mods
# Game Status: connecting/paused/running
# Chatbot connecting/connected/disconnected
# Buttons: Reload mods
#          Quit

# Modifier Settings

voting_frame = sg.Frame('Voting',[
    [sg.Text('Voting method:' size=(25,1)),
     sg.Combo(['Proportional','Majority','None'],
              default_value=sg.user_settings_get_entry('-voting method-', 'Proportional'),
              tooltip='What method should be used to determine the winner. "None" disables voting.')],
    [sg.Text('Repeat Mod Probability (%):',size=(25,1)), sg.InputText(key='-mod prob-')],
    [sg.Text('Reset mod history'), sg.Button('Clear', tooltip='Reset memory of which mods have already been used')],
    [sg.Text('Announce winner in chat:'),
     sg.Button(image_data=toggle_btn_off, key='-TOGGLE winners-', border_width=0,
               button_color=(sg.theme_background_color(), sg.theme_background_color()),
               disabled_button_color=(sg.theme_background_color(), sg.theme_background_color()),
               tooltip='Should the mod that wins the vote be announced in chat?',
               metadata=BtnInfo())],
    [sg.Text('Announce vote candidates in chat:'),
     sg.Button(image_data=toggle_btn_off, key='-TOGGLE voting-', border_width=0,
               button_color=(sg.theme_background_color(), sg.theme_background_color()),
               disabled_button_color=(sg.theme_background_color(), sg.theme_background_color()),
               tooltip='If on, the bot will announce the mods you can vote for in chat at the start of each voting cycle',
               metadata=BtnInfo())]])

redemption_frame = sg.Frame('Mod Redemption',[
    [sg.Button(image_data=toggle_btn_off, key='-TOGGLE points redeem-', border_width=0,
               button_color=(sg.theme_background_color(), sg.theme_background_color()),
               disabled_button_color=(sg.theme_background_color(), sg.theme_background_color()),
               tooltip='Allow viewers to apply a mod by redeeming channel points',
               metadata=BtnInfo()),
     sg.Text('Points Redemption')],
    [sg.Text('Bits Redemption'),
     sg.Button(image_data=toggle_btn_off, key='-TOGGLE bits redeem-', border_width=0,
               button_color=(sg.theme_background_color(), sg.theme_background_color()),
               disabled_button_color=(sg.theme_background_color(), sg.theme_background_color()),
               tooltip='Allow viewers to apply a mod by giving bits',
               metadata=BtnInfo())]
    [sg.Text('A redemption should:'), sg.Combo(['Replace oldest mod','Be an extra mod'])
])

mod_col1 = sg.Column([
    [sg.Text('Active Mods:', size=(25,1)), sg.
    [sg.Text('Time per modifier (sec):', size=(25,1)), sg.InputText(key='-mod time-', tooltip='How long should a mod remain active before it is removed')],
    voting_frame, redemption_frame],pad=(0,0))



modifier_layout = [sg.vtop([mod_col1, mod_col2])]
                   

        ]]
    [sg.Frame('Modifiers',[

        [sg.Text('Available Modifiers:')]
        ])],
                   
]

# Available Mods
#  - Name (groups) Active (check) Redeemable (check) 
# Allow mod redemption (channel points, bits, donations)


# Vote Settings
# Voting method: proportional/majority/disabled

# Selection Weighting:
# repeat mod probability
# reset mod memory
# Limit by mod group

# Appearance of overlays and 
settings_layout = [[sg.Text('Dark Mode'),
                    sg.Button(image_data=toggle_btn_off, k='-TOGGLE-darkmode-', border_width=0,
                              button_color=(sg.theme_background_color(), sg.theme_background_color()),
                              disabled_button_color=(sg.theme_background_color(), sg.theme_background_color()),
                              metadata=BtnInfo())],
                   [sg.Text('Browser Update Rate (Hz):', size=(25,1)), sg.InputText(key='-refresh-')]
                   ]
# UI refresh rate

connection

layout = [[sg.Tab('Streaming View', streaming_layout, key='-streamer-'),
           sg.Tab('Modifiers', modifier_layout, key='-modifiers-'),
           sg.Tab('Appearance', appearance_layout, key='-appearance-'),
           sg.Tab('Connection', connection_layout, key='-connection-')]]

window = sg.Window('Twitch Controls Chaos', layout, default_element_size=(12, 1))

while True:
    event, values = window.read()
    sg.popup_non_blocking(event, values)
    if event == sg.WIN_CLOSED:
        break
                    
    if 'TOGGLE' in event:
        window[event].metadata.state = not window[event].metadata.state
        window[event].update(image_data=toggle_btn_on if window[event].metadata.state else toggle_btn_off)

        
window.close()
