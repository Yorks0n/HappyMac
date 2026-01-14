function buildConfigUrl() {
  var html = "" +
    "<!DOCTYPE html>" +
    "<html>" +
    "<head>" +
    "<meta name='viewport' content='width=device-width, initial-scale=1'>" +
    "<title>HappyMac Theme</title>" +
    "<style>" +
    "body{font-family:Helvetica,Arial,sans-serif;background:#f6f2e9;color:#1b1b1b;margin:0;padding:20px;}" +
    "h1{font-size:20px;margin:0 0 16px 0;}" +
    "label{display:block;font-size:14px;margin-bottom:8px;}" +
    "select{width:100%;font-size:16px;padding:10px;border:1px solid #c7c1b6;border-radius:6px;background:#fff;}" +
    ".row{margin-top:20px;display:flex;gap:10px;}" +
    "button{flex:1;font-size:16px;padding:10px;border-radius:6px;border:1px solid #1b1b1b;background:#1b1b1b;color:#fff;}" +
    "button.secondary{background:#fff;color:#1b1b1b;border-color:#1b1b1b;}" +
    "</style>" +
    "</head>" +
    "<body>" +
    "<h1>Theme</h1>" +
    "<label for='theme'>Choose a watchface theme</label>" +
    "<select id='theme'>" +
    "<option value='0'>Light</option>" +
    "<option value='1'>Dark</option>" +
    "<option value='2'>Color</option>" +
    "</select>" +
    "<div class='row'>" +
    "<button type='button' class='secondary' id='cancel'>Cancel</button>" +
    "<button type='button' id='save'>Save</button>" +
    "</div>" +
    "<script>" +
    "var themeEl=document.getElementById('theme');" +
    "document.getElementById('cancel').addEventListener('click',function(){" +
    "document.location='pebblejs://close#';" +
    "});" +
    "document.getElementById('save').addEventListener('click',function(){" +
    "var payload={theme:parseInt(themeEl.value,10)};" +
    "document.location='pebblejs://close#'+encodeURIComponent(JSON.stringify(payload));" +
    "});" +
    "</script>" +
    "</body>" +
    "</html>";

  return "data:text/html," + encodeURIComponent(html);
}

Pebble.addEventListener("showConfiguration", function() {
  Pebble.openURL(buildConfigUrl());
});

Pebble.addEventListener("webviewclosed", function(e) {
  if (!e || !e.response) {
    return;
  }

  var config = {};
  var response = e.response;
  try {
    if (response.charAt(0) === "{") {
      config = JSON.parse(response);
    } else {
      config = JSON.parse(decodeURIComponent(response));
    }
  } catch (err) {
    return;
  }

  if (typeof config.theme === "number" && !isNaN(config.theme)) {
    Pebble.sendAppMessage({
      theme: config.theme
    });
  }
});
