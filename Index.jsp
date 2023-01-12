<%@ page contentType="text/html; charset=utf-8" language="java" import="java.io.*, java.util.*, java.sql.*, javax.xml.parsers.DocumentBuilderFactory,javax.xml.parsers.DocumentBuilder,org.w3c.dom.*" errorPage="" 
%>
<!doctype html>
<html>
<head>
    <meta charset="utf-8" />
    <title>Fire Fighter Monitoring Tool</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />

    <link rel="stylesheet" type="text/css" href="css/bootstrap.min.css" />
    <link rel="stylesheet" type="text/css" href="css/font-awesome.min.css" />

    <script type="text/javascript" src="js/jquery-1.10.2.min.js"></script>
    <script type="text/javascript" src="bootstrap/js/bootstrap.min.js"></script>
</head>
<style>
    .map {
        min-width: 300px;
        min-height: 300px;
        width: 100%;
        height: 100%;
    }

    .header {
        background-color: #F5F5F5;
        color: #36A0FF;
        height: 70px;
        font-size: 27px;
        padding: 10px;
    }
</style>
<body>

<%
response.setIntHeader("Refresh", 1);
// Get current time
Calendar calendar = new GregorianCalendar();
String am_pm;
int hour = calendar.get(Calendar.HOUR);
int minute = calendar.get(Calendar.MINUTE);
int second = calendar.get(Calendar.SECOND);

	DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
	
	DocumentBuilder db = dbf.newDocumentBuilder();

	Document doc = db.parse("http://localhost:8080/FireFighter/data.xml");
	
	NodeList critical = doc.getElementsByTagName("critical");
	NodeList heartRate = doc.getElementsByTagName("heart");
	NodeList ogygenLevel = doc.getElementsByTagName("oxygen");
	NodeList latitude = doc.getElementsByTagName("latitude");
	NodeList longitude = doc.getElementsByTagName("longitude");
	%>

<nav class="navbar navbar-inverse">
  <div class="container-fluid"> 
    <!-- Brand and toggle get grouped for better mobile display -->
    <div class="navbar-header">
      <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#myInverseNavbar2"> <span class="sr-only">Toggle navigation</span> <span class="icon-bar"></span> <span class="icon-bar"></span> <span class="icon-bar"></span> </button>
      <a class="navbar-brand" href="index.html">Fire Fighter Active Monitoring </a> 
    </div>
    <!-- Collect the nav links, forms, and other content for toggling -->
    <div class="collapse navbar-collapse" id="myInverseNavbar2">
      <ul class="nav navbar-nav navbar-right">
        <li><a href="faq.jsp">FAQ</a></li>
      </ul>
    </div>
    <!-- /.navbar-collapse --> 
  </div>
  <!-- /.container-fluid --> 
</nav>

<div class="container">


<!-- Contact with Map - START -->
<center>
<div class="container">
    <div class="row">
        <div class="col-md-6">
        <div class="panel panel-default">
<div class="panel-body text-center">
<%
if(calendar.get(Calendar.AM_PM) == 0)
   am_pm = "AM";
else
   am_pm = "PM";
String CT = hour+":"+ minute +":"+ second +" "+ am_pm;
out.println("Current Time: " + CT + "\n");

%>
</div>
</div>
    </div>
        </div>
        </div>
        </center>

<div class="container">
    <div class="row">
        <div class="col-md-6">
        <div class="panel panel-default">
                    <fieldset>
                        <legend class="text-center header">Status </legend>
                        <div class="panel-body text-center">
                        <h2>
                        <% 
                        int status = Integer.parseInt(critical.item(0).getFirstChild().getNodeValue().trim());
                        if ( status == 1) {
                        	out.print("Danger !");
                        }
                        	else{
                        		out.print("Normal");
                        	}
                        %>
                        </h2>
                        </div>
                    </fieldset>
        </div>
        </div>
        <div class="col-md-6">
                <div class="panel panel-default">
                    <fieldset>
                        <legend class="text-center header">Oxygen </legend>
                        <div class="panel-body text-center">
                        <h2>
                        <%= ogygenLevel.item(0).getFirstChild().getNodeValue()%> %
                        </h2>
                        </div>
                    </fieldset>
        </div>
        </div>
        <div class="col-md-6">
        <div class="panel panel-default">
                    <fieldset>
                        <legend class="text-center header">Heart Rate </legend>
                        <div class="panel-body text-center">
                        <img class="item" src="img/heartGreen.jpg" alt="placeholder image">
                        <h2>
                        <%= heartRate.item(0).getFirstChild().getNodeValue()%>
                        </h2>
                        </div>
                    </fieldset>
        </div>
        </div>

        <div class="col-md-6">
            <div>
                <div class="panel panel-default">
                    <div class="text-center header">Location </div>
                    <div class="panel-body text-center">
                        <div>
                        Latitude: <%= latitude.item(0).getFirstChild().getNodeValue()%><br/>
                        Longitude: <%= longitude.item(0).getFirstChild().getNodeValue()%> <br/>
                        </div>
                        <hr />
                        <div id="map1" class="map">
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</div>


<script src="http://maps.google.com/maps/api/js?key=YOUR_MAP_API_KEY&sensor=false"></script>
<script type="text/javascript">
    jQuery(function ($) {
        function init_map1() {
            var myLocation = new google.maps.LatLng(<%= latitude.item(0).getFirstChild().getNodeValue()%>, <%= longitude.item(0).getFirstChild().getNodeValue()%>);
            var mapOptions = {
                center: myLocation,
                zoom: 18
            };
            var marker = new google.maps.Marker({
                position: myLocation,
                title: "FireFighter"
            });
            var map = new google.maps.Map(document.getElementById("map1"),
                mapOptions);
            marker.setMap(map);
        }
        init_map1();
    });
</script>



<!-- Contact with Map - END -->

</div>

</body>
</html>
