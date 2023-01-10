var myApp = angular.module('fireFighter',[]);

myApp.controller('fireFighterController', ['$scope', '$http', '$interval', function($scope, $http, $interval) {
	
	$scope.status = "Normal";
	$scope.monitoringData = [];
	this.$onInit = function() {
		$interval(getData, 200);
		//getData();
		$interval(setData, 200);
	}
	isCritical = function(item) {
		return item == 1 ? "Danger!" : "Safe";
	}
	
	getData = function() {
		$http.get("data.json").then(function(response) {
		    response.data.forEach(function(monitoringInfo) {
		    	$scope.monitoringData.push(monitoringInfo)
		    });
//		    $scope.name.push(monitoringInfo.name);
//	    	$scope.critical.push(monitoringInfo.critical);
//	    	$scope.heart.push(monitoringInfo.heart);
//	    	$scope.oxygen.push(monitoringInfo.oxygen);
//	    	$scope.latitude.push(monitoringInfo.latitude);
//	    	$scope.longitude.push(monitoringInfo.longitude);
//		    console.log($scope.heart);
//		    $scope.name = response.data.name;
//			$scope.critical = response.data.critical;
//			$scope.heart = response.data.heart;
//		 	$scope.oxygen = response.data.oxygen;
//			$scope.latitude = response.data.latitude;
//			$scope.longitude = response.data.longitude;
//			$scope.status = isCritical($scope.critical);
		    if($scope.monitoringData.length > 20) {
		    	$scope.monitoringData.splice(0, 5);
		    }
		   // console.log($scope.monitoringData);
		});
	}
	setData = function() {
		$scope.name = $scope.monitoringData[0].name;
		$scope.source = $scope.monitoringData[0].source;
		$scope.critical = $scope.monitoringData[0].critical;
		$scope.heart = $scope.monitoringData[0].heart;
	 	$scope.oxygen = $scope.monitoringData[0].oxygen;
	 	$scope.toxic = $scope.monitoringData[0].toxic;
		$scope.latitude = $scope.monitoringData[0].latitude;
		$scope.longitude = $scope.monitoringData[0].longitude;
		$scope.status = isCritical($scope.critical);
		$scope.monitoringData.splice(0, 1);
	}
	$scope.inDanger = function() {
		return $scope.status == 'Danger!' ? true:false;
	} 
	
}]);
