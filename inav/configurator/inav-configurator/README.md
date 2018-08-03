# INAV Configurator

INAV Configurator is a crossplatform configuration tool for the [INAV](https://github.com/iNavFlight/inav) flight control system.

It runs as an app within Google Chrome and allows you to configure the INAV software running on any supported INAV target.

Various types of aircraft are supported by the tool and by INAV, e.g. quadcopters, hexacopters, octocopters and fixed-wing aircraft.

## INAV Configurator start minimized, what should I do?

You have to remove `C:\Users%Your_UserNname%\AppData\Local\inav-configurator` folder and all its content.

[https://www.youtube.com/watch?v=XMoULyiFDp4](https://www.youtube.com/watch?v=XMoULyiFDp4)

## Installation

Depending on target operating system, _INAV Configurator_ is distributed as _standalone_ application or Chrome App.

### Windows

1. Visit [release page](https://github.com/iNavFlight/inav-configurator/releases)
1. Download Configurator for Windows platform (win32 or win64 is present)
1. Extract ZIP archive
1. Run INAV Configurator app from unpacked folder
1. Configurator is not signed, so you have to allow Windows to run untrusted application. There might be a monit for it during first run 

### Mac

1. Visit [release page](https://github.com/iNavFlight/inav-configurator/releases)
1. Download Configurator for Mac platform
1. Extract ZIP archive
1. Run INAV Configurator
1. Configurator is not signed, so you have to allow Mac to run untrusted application. There might be a monit for it during first run 

### ChromeOS

**INAV Configurator** form ChromeOS is available in [Chrome Web Store](https://chrome.google.com/webstore/detail/inav-configurator/fmaidjmgkdkpafmbnmigkpdnpdhopgel)

### Building and running INAV Configurator locally (for development or Linux users)

For local development, **node.js** build system is used.

1. Install node.js
1. From project folder run `npm install`
1. To build the JS and CSS files and start the configurator:
    - With NW.js: Run `npm start`.
    - With Chrome: Run `./node_modules/gulp/bin/gulp.js`. Then open `chrome://extensions`, enable
    the `Developer mode`, click on the `Load unpacked extension...` button and select the `inav-configurator` directory.

Other tasks are also defined in `gulpfile.js`. To run a task, use `./node_modules/gulp/bin/gulp.js task-name`. Available ones are:

- **build**: Generate JS and CSS output files used by the configurator from their sources. It must be run whenever changes are made to any `.js` or `.css` files in order to have those changes appear
in the configurator. If new files are added, they must be included in `gulpfile.js`. See the comments at the top of `gulpfile.js` to learn how to do so. See also the `watch` task.
- **watch**: Watch JS and CSS sources for changes and run the `build` task whenever they're edited.
- **dist**: Create a distribution of the app (valid for packaging both as a Chrome app or a NW.js app)
in the `./dist/` directory.
- **release**: Create NW.js apps for each supported platform (win32, osx64 and linux64) in the `./apps`
directory. Running this task on macOS or Linux requires Wine, since it's needed to set the icon
for the Windows app. If you don't have Wine installed you can create a release by running the **release-only-linux** task.

## Authors

Konstantin Sharlaimov/DigitalEntity - maintainer of the INAV firmware and configurator.

INAV Configurator was originally a [fork](#credits) of Cleanflight Configurator with support for INAV instead of Cleanflight.

This configurator is the only configurator with support for INAV specific features. It will likely require that you run the latest firmware on the flight controller.
If you are experiencing any problems please make sure you are running the [latest firmware version](https://github.com/iNavFlight/inav/releases).

## Notes

### WebGL

Make sure Settings -> System -> "User hardware acceleration when available" is checked to achieve the best performance

### Linux users

1. Dont forget to add your user into dialout group "sudo usermod -aG dialout YOUR_USERNAME" for serial access
2. If you have 3D model animation problems, enable "Override software rendering list" in Chrome flags chrome://flags/#ignore-gpu-blacklist

## Support

GitHub issue tracker is reserved for bugs and other technical problems. If you do not know how to setup
everything, hardware is not working or have any other _support_ problem, please consult:

* [rcgroups main thread](https://www.rcgroups.com/forums/showthread.php?2495732-Cleanflight-iNav-(navigation-rewrite)-project)
* [Slack channel](https://inavflight.signup.team/)

## Issue trackers

For INAV configurator issues raise them here

https://github.com/iNavFlight/inav-configurator/issues

For INAV firmware issues raise them here

https://github.com/iNavFlight/inav/issues

## Developers

We accept clean and reasonable patches, submit them!

## Credits

ctn - primary author and maintainer of Baseflight Configurator.
Hydra - author and maintainer of Cleanflight Configurator from which this project was forked.